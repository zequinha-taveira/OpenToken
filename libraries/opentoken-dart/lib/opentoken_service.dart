import 'dart:convert';
import 'dart:typed_data';

/// Interface for physical communication with the OpenToken device.
/// Can be implemented for NFC (Mobile) or USB (Desktop/Web).
abstract class OpenTokenTransport {
  Future<Uint8List> sendApdu(Uint8List apdu);
  Future<Uint8List> sendCtapHid(int command, Uint8List payload);
}

/// Shared Protocol Service for OpenToken (Cross-Platform)
class OpenTokenSharedService {
  final OpenTokenTransport transport;

  static const List<int> OATH_AID = [
    0xA0,
    0x00,
    0x00,
    0x05,
    0x27,
    0x21,
    0x01,
    0x01
  ];
  static const int CTAP2_CMD_GET_INFO = 0x04;
  static const int CTAP2_CMD_CRED_MGMT = 0x41;

  // Management Commands (Proprietary/WebUSB)
  static const int MGMT_CMD_GET_VERSION = 0x01;
  static const int MGMT_CMD_REBOOT_BOOTLOADER = 0x06;

  OpenTokenSharedService(this.transport);

  /// --- OATH Operations ---

  /// Connects to the OATH applet
  Future<bool> selectOathApplet() async {
    final apdu = Uint8List.fromList(
        [0x00, 0xA4, 0x04, 0x00, OATH_AID.length, ...OATH_AID]);
    final resp = await transport.sendApdu(apdu);
    return resp.length >= 2 && resp[resp.length - 2] == 0x90;
  }

  /// Lists OATH accounts
  Future<List<Map<String, String>>> listOathAccounts() async {
    final apdu = Uint8List.fromList([0x00, 0xA1, 0x00, 0x00, 0x00]);
    final resp = await transport.sendApdu(apdu);
    return _parseAccountList(resp);
  }

  /// Calculates a TOTP code
  Future<String?> calculateCode(String accountName) async {
    final nameBytes = utf8.encode(accountName);
    final apdu = Uint8List.fromList([
      0x00,
      0xA2,
      0x00,
      0x01,
      nameBytes.length + 2,
      0x71,
      nameBytes.length,
      ...nameBytes,
      0x00
    ]);
    final resp = await transport.sendApdu(apdu);

    if (resp.length >= 5 && resp[resp.length - 2] == 0x90) {
      // Parse 0x76 0x05 0x06 [4 bytes code]
      final code = (resp[3] << 24) | (resp[4] << 16) | (resp[5] << 8) | resp[6];
      return (code % 1000000).toString().padLeft(6, '0');
    }
    return null;
  }

  /// --- FIDO2 Operations ---

  /// Retrieves Authenticator Info
  Future<Map<dynamic, dynamic>?> getFidoInfo() async {
    // CTAP2_GET_INFO = 0x04
    final resp = await transport.sendCtapHid(CTAP2_CMD_GET_INFO, Uint8List(0));
    // Implementation would require a CBOR decoder in Dart
    return null; // Placeholder until CBOR is added
  }

  /// --- Management Operations ---

  /// Gets the firmware version from the device
  Future<String> getFirmwareVersion() async {
    try {
      final resp =
          await transport.sendCtapHid(MGMT_CMD_GET_VERSION, Uint8List(0));
      if (resp.length >= 4 && resp[0] == 0x00) {
        return "${resp[1]}.${resp[2]}.${resp[3]}";
      }
    } catch (e) {
      return "Unknown";
    }
    return "Unknown";
  }

  /// Triggers a reboot into BOOTSEL mode
  Future<bool> enterBootloaderMode() async {
    try {
      final resp =
          await transport.sendCtapHid(MGMT_CMD_REBOOT_BOOTLOADER, Uint8List(0));
      return resp.isNotEmpty && resp[0] == 0x00;
    } catch (e) {
      return false;
    }
  }

  /// --- Helper Parsers ---

  List<Map<String, String>> _parseAccountList(Uint8List response) {
    final List<Map<String, String>> result = [];
    int i = 0;
    while (i < response.length - 2) {
      if (response[i] == 0x72) {
        int len = response[i + 1];
        int j = i + 2;
        int end = j + len;
        String? name;
        String type = "TOTP";
        while (j < end) {
          int tag = response[j];
          int tagLen = response[j + 1];
          if (tag == 0x71) {
            name = utf8.decode(response.sublist(j + 2, j + 2 + tagLen));
          } else if (tag == 0x75) {
            type = (response[j + 2] & 0x20 != 0) ? "TOTP" : "HOTP";
          }
          j += 2 + tagLen;
        }
        if (name != null) result.add({"name": name, "type": type});
        i += 2 + len;
      } else {
        i++;
      }
    }
    return result;
  }
}
