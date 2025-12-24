import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:win32/win32.dart';

import '../opentoken_service.dart';
import 'windows_hid_driver.dart';

/// OpenToken USB Device identifiers
const int OPENTOKEN_VID = 0x1209; // pid.codes - Open Source Hardware VID
const int OPENTOKEN_PID = 0x0001; // Generic security token PID

/// USB HID Transport for Windows Desktop
/// Communicates with the RP2350 firmware via USB HID
class UsbTransport implements OpenTokenTransport {
  int _deviceHandle = 0;
  bool _isConnected = false;
  String? _devicePath;

  final int vendorId;
  final int productId;

  UsbTransport({
    this.vendorId = OPENTOKEN_VID,
    this.productId = OPENTOKEN_PID,
  });

  // Connection state stream for UI updates
  final StreamController<bool> _connectionController =
      StreamController<bool>.broadcast();
  Stream<bool> get connectionStream => _connectionController.stream;
  bool get isConnected => _isConnected;

  /// Find all connected OpenToken devices
  Future<List<String>> findDevices() async {
    if (!Platform.isWindows) {
      throw UnsupportedError('USB Transport only supported on Windows');
    }

    final devices = <String>[];

    // Get HID device interface GUID
    final hidGuid = calloc<GUID>();
    HidD_GetHidGuid(hidGuid);

    // Get device info set
    final deviceInfoSet = SetupDiGetClassDevs(
      hidGuid,
      nullptr,
      NULL,
      DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
      calloc.free(hidGuid);
      return devices;
    }

    try {
      final deviceInterfaceData = calloc<SP_DEVICE_INTERFACE_DATA>();
      deviceInterfaceData.ref.cbSize = sizeOf<SP_DEVICE_INTERFACE_DATA>();

      int deviceIndex = 0;

      while (SetupDiEnumDeviceInterfaces(
            deviceInfoSet,
            nullptr,
            hidGuid,
            deviceIndex,
            deviceInterfaceData,
          ) ==
          TRUE) {
        deviceIndex++;

        // Get required buffer size
        final requiredSize = calloc<DWORD>();
        SetupDiGetDeviceInterfaceDetail(
          deviceInfoSet,
          deviceInterfaceData,
          nullptr,
          0,
          requiredSize,
          nullptr,
        );

        if (requiredSize.value == 0) {
          calloc.free(requiredSize);
          continue;
        }

        // Allocate and get device interface detail
        final detailData = calloc<Uint8>(requiredSize.value);
        final detailDataPtr =
            detailData.cast<SP_DEVICE_INTERFACE_DETAIL_DATA_>();
        // On x64 Windows, cbSize is 8 bytes.
        // On x32 Windows, it would be 4 + sizeof(WCHAR).
        // SetupDiGetDeviceInterfaceDetail requires this to be set correctly.
        detailDataPtr.ref.cbSize = (sizeOf<IntPtr>() == 8) ? 8 : 6;

        if (SetupDiGetDeviceInterfaceDetail(
              deviceInfoSet,
              deviceInterfaceData,
              detailDataPtr,
              requiredSize.value,
              nullptr,
              nullptr,
            ) ==
            TRUE) {
          // Extract device path
          final pathPtr = Pointer<Utf16>.fromAddress(
            detailDataPtr.address + 4, // OFFSET is always 4 bytes after cbSize
          );
          final devicePath = pathPtr.toDartString();

          // Specifically look for Interface 1 (&mi_01) - This is the FIDO2/CTAP interface
          // Interface 0 is the Keyboard, which doesn't support our custom tunnel.
          if (devicePath.toLowerCase().contains('&mi_01')) {
            if (await _isOpenTokenDevice(devicePath)) {
              devices.add(devicePath);
            }
          }
        }

        calloc.free(detailData);
        calloc.free(requiredSize);
      }

      calloc.free(deviceInterfaceData);
    } finally {
      SetupDiDestroyDeviceInfoList(deviceInfoSet);
      calloc.free(hidGuid);
    }

    return devices;
  }

  /// Check if a device path is an OpenToken device
  Future<bool> _isOpenTokenDevice(String devicePath) async {
    final pathPtr = devicePath.toNativeUtf16();
    final handle = CreateFile(
      pathPtr,
      0, // ACCESS: 0 (NULL access) allows getting attributes without locking the device
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      0,
      NULL,
    );
    calloc.free(pathPtr);

    if (handle == INVALID_HANDLE_VALUE) {
      return false;
    }

    try {
      final attrs = calloc<HIDD_ATTRIBUTES>();
      attrs.ref.Size = sizeOf<HIDD_ATTRIBUTES>();

      if (HidD_GetAttributes(handle, attrs) == TRUE) {
        final vid = attrs.ref.VendorID;
        final pid = attrs.ref.ProductID;
        calloc.free(attrs);

        return vid == vendorId && pid == productId;
      }

      calloc.free(attrs);
    } finally {
      CloseHandle(handle);
    }

    return false;
  }

  /// Connect to the first available OpenToken device
  Future<bool> connect() async {
    if (_isConnected) return true;

    final devices = await findDevices();
    if (devices.isEmpty) {
      return false;
    }

    return connectToDevice(devices.first);
  }

  /// Connect to a specific device
  Future<bool> connectToDevice(String devicePath) async {
    if (_isConnected && _devicePath == devicePath) return true;

    // Close existing connection
    if (_isConnected) {
      disconnect();
    }

    final pathPtr = devicePath.toNativeUtf16();
    _deviceHandle = CreateFile(
      pathPtr,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      0,
      NULL,
    );
    calloc.free(pathPtr);

    if (_deviceHandle == INVALID_HANDLE_VALUE) {
      _deviceHandle = 0;
      return false;
    }

    _devicePath = devicePath;
    _isConnected = true;
    _connectionController.add(true);

    return true;
  }

  /// Disconnect from the device
  void disconnect() {
    if (_deviceHandle != 0) {
      CloseHandle(_deviceHandle);
      _deviceHandle = 0;
    }
    _isConnected = false;
    _devicePath = null;
    _connectionController.add(false);
  }

  /// Get device serial number string
  Future<String?> getSerialNumber() async {
    if (!_isConnected ||
        _deviceHandle == 0 ||
        _deviceHandle == INVALID_HANDLE_VALUE) {
      return null;
    }

    // Typical serial numbers are Short (12-32 chars)
    final bufferSize = 256;
    final buffer = calloc<Uint8>(bufferSize);

    try {
      if (HidD_GetSerialNumberString(
          _deviceHandle, buffer.cast<Void>(), bufferSize)) {
        return buffer.cast<Utf16>().toDartString();
      }
    } catch (e) {
      print('Error getting serial number: $e');
    } finally {
      calloc.free(buffer);
    }

    return null;
  }

  /// Send APDU via CCID encapsulation
  @override
  Future<Uint8List> sendApdu(Uint8List apdu) async {
    // Send APDU using the custom Tunnel command (0x70) over CTAPHID
    return sendCtapHid(0x70, apdu);
  }

  /// Send CTAP HID / WebUSB command
  @override
  Future<Uint8List> sendCtapHid(int command, Uint8List payload) async {
    if (!_isConnected) {
      throw StateError('Device not connected');
    }

    // CCID/CTAP Headers are mandatory
    // Packet: [CID(4), CMD(1), LEN(2), PAYLOAD(57)] = 64 bytes

    final packet = Uint8List(64);
    final data = ByteData.view(packet.buffer);

    // CID: Use Broadcast (0xFFFFFFFF) for simplicity in this driverless mode
    // In a full implementation, we would perform INIT and get a CID.
    data.setUint32(0, 0xFFFFFFFF, Endian.little);

    // CMD: Set the Init flag (0x80)
    data.setUint8(4, command | 0x80);

    // LEN: Payload length (Big Endian)
    data.setUint16(5, payload.length, Endian.big);

    // PAYLOAD
    int bytesToCopy = payload.length;
    if (bytesToCopy > 57) {
      bytesToCopy = 57; // Limit to single packet for this implementation
    }

    packet.setRange(7, 7 + bytesToCopy, payload.sublist(0, bytesToCopy));

    // Write to device
    final responseWithHeader = await _hidWrite(packet);

    // Parse Response
    // [CID(4), CMD(1), LEN(2), DATA...]
    if (responseWithHeader.length < 7) {
      return Uint8List(0);
    }

    // Extract length
    final respData = ByteData.view(responseWithHeader.buffer);
    int dataLen = respData.getUint16(5, Endian.big);

    // Safety clamp
    if (dataLen > responseWithHeader.length - 7) {
      dataLen = responseWithHeader.length - 7;
    }

    return responseWithHeader.sublist(7, 7 + dataLen);
  }

  /// Low-level HID write/read
  Future<Uint8List> _hidWrite(Uint8List data) async {
    // Prepare output report (64 bytes, first byte is report ID)
    final reportSize = 65;
    final buffer = calloc<Uint8>(reportSize);
    buffer[0] = 0x00; // Report ID

    for (var i = 0; i < data.length && i < 64; i++) {
      buffer[i + 1] = data[i];
    }

    // Write to HID device
    final bytesWritten = calloc<DWORD>();
    final writeResult = WriteFile(
      _deviceHandle,
      buffer,
      reportSize,
      bytesWritten,
      nullptr,
    );

    calloc.free(bytesWritten);

    if (writeResult == FALSE) {
      calloc.free(buffer);
      throw StateError('HID write failed: ${GetLastError()}');
    }

    // Read response
    final readBuffer = calloc<Uint8>(reportSize);
    final bytesRead = calloc<DWORD>();

    final readResult = ReadFile(
      _deviceHandle,
      readBuffer,
      reportSize,
      bytesRead,
      nullptr,
    );

    if (readResult == FALSE) {
      calloc.free(buffer);
      calloc.free(readBuffer);
      calloc.free(bytesRead);
      throw StateError('HID read failed: ${GetLastError()}');
    }

    // Copy response data
    final responseLength = bytesRead.value;
    final response = Uint8List(responseLength > 0 ? responseLength - 1 : 0);
    for (var i = 0; i < response.length; i++) {
      response[i] = readBuffer[i + 1]; // Skip report ID
    }

    calloc.free(buffer);
    calloc.free(readBuffer);
    calloc.free(bytesRead);

    return response;
  }

  /// Dispose resources
  void dispose() {
    disconnect();
    _connectionController.close();
  }
}
