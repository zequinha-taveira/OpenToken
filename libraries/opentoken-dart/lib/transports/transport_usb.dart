import 'dart:typed_data';
import '../opentoken_service.dart';

/// Mock implementation of OpenTokenTransport for Desktop USB devices.
/// A real implementation would use a package like 'hid' or 'win32' to talk to the RP2350.
class UsbTransport implements OpenTokenTransport {
  @override
  Future<Uint8List> sendApdu(Uint8List apdu) async {
    // In a real desktop app, this would wrap the APDU for CCID
    print("USB Desktop: Sending APDU ${apdu.length} bytes");
    return Uint8List.fromList([0x90, 0x00]); // Success mock
  }

  @override
  Future<Uint8List> sendCtapHid(int command, Uint8List payload) async {
    print(
        "USB Desktop: Sending CTAPHID Command 0x${command.toRadixString(16)}");
    return Uint8List.fromList([0x00]); // Status OK mock
  }
}
