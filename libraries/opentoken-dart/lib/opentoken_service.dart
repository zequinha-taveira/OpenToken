import 'dart:convert';
import 'dart:typed_data';

import 'package:cbor/cbor.dart';
import 'package:crypto/crypto.dart';

/// Base class for OpenToken related exceptions
class OpenTokenException implements Exception {
  final String message;
  final int? statusWord;
  OpenTokenException(this.message, {this.statusWord});
  @override
  String toString() =>
      'OpenTokenException: $message' +
      (statusWord != null ? ' (0x${statusWord!.toRadixString(16)})' : '');
}

/// Thrown when an operation requires a PIN but none was provided or it's locked
class PinRequiredException extends OpenTokenException {
  PinRequiredException()
      : super('PIN required to perform this operation', statusWord: 0x6982);
}

/// Interface for physical communication with the OpenToken device.
/// Can be implemented for NFC (Mobile) or USB (Desktop/Web).
abstract class OpenTokenTransport {
  Future<Uint8List> sendApdu(Uint8List apdu);
  Future<Uint8List> sendCtapHid(int command, Uint8List payload);
}

/// OATH credential type
enum OathType { totp, hotp }

/// OATH hash algorithm
enum OathAlgorithm { sha1, sha256, sha512 }

/// Result of an OATH calculation
class OathCode {
  final String code;
  final int digits;
  final int validSeconds;

  OathCode({required this.code, this.digits = 6, this.validSeconds = 30});
}

/// Device status information
class DeviceStatus {
  final int fido2Count;
  final int fido2Max;
  final int oathCount;
  final int oathMax;

  DeviceStatus({
    required this.fido2Count,
    required this.fido2Max,
    required this.oathCount,
    required this.oathMax,
  });

  double get oathUsagePercent => oathMax > 0 ? oathCount / oathMax : 0;
  double get fido2UsagePercent => fido2Max > 0 ? fido2Count / fido2Max : 0;
}

/// FIDO2 Resident Key Credential
class FidoCredential {
  final String rpId;
  final String userDisplayName;
  final String userName;
  final Uint8List credentialId;

  FidoCredential({
    required this.rpId,
    required this.userDisplayName,
    required this.userName,
    required this.credentialId,
  });

  @override
  String toString() => 'FidoCredential(rpId: $rpId, user: $userName)';
}

/// Shared Protocol Service for OpenToken (Cross-Platform)
class OpenTokenSharedService {
  final OpenTokenTransport transport;

  // OATH Application ID
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

  // OATH Instructions
  static const int OATH_INS_PUT = 0x01;
  static const int OATH_INS_DELETE = 0x02;
  static const int OATH_INS_SET_CODE = 0x03;
  static const int OATH_INS_RESET = 0x04;
  static const int OATH_INS_LIST = 0xA1;
  static const int OATH_INS_CALCULATE = 0xA2;
  static const int OATH_INS_VALIDATE = 0xA3;
  static const int OATH_INS_CALCULATE_ALL = 0xA4;

  // OATH TLV Tags
  static const int TAG_NAME = 0x71;
  static const int TAG_NAME_LIST = 0x72;
  static const int TAG_KEY = 0x73;
  static const int TAG_CHALLENGE = 0x74;
  static const int TAG_PROPERTY = 0x75;
  static const int TAG_RESPONSE = 0x76;
  static const int TAG_NO_RESP = 0x77;

  // CTAP2 Commands
  static const int CTAP2_CMD_GET_INFO = 0x04;
  static const int CTAP2_CMD_CRED_MGMT = 0x41;

  // WebUSB Management Commands
  static const int MGMT_CMD_GET_VERSION = 0x01;
  static const int MGMT_CMD_LIST_CREDS = 0x02;
  static const int MGMT_CMD_DELETE_CRED = 0x03;
  static const int MGMT_CMD_GET_STATUS = 0x04;
  static const int MGMT_CMD_RESET_DEVICE = 0x05;
  static const int MGMT_CMD_REBOOT_BOOTLOADER = 0x06;
  static const int MGMT_CMD_LIST_OATH = 0x10;
  static const int MGMT_CMD_DELETE_OATH = 0x11;

  // CTAP2 Sub-commands
  static const int CTAP2_SUB_GET_RP = 0x02;
  static const int CTAP2_SUB_GET_CRED = 0x04;

  OpenTokenSharedService(this.transport);

  // ═══════════════════════════════════════════════════════════════════════════
  // OATH OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Select the OATH applet
  Future<bool> selectOathApplet() async {
    final apdu = Uint8List.fromList(
        [0x00, 0xA4, 0x04, 0x00, OATH_AID.length, ...OATH_AID]);
    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  /// List all OATH accounts on the device
  Future<List<Map<String, String>>> listOathAccounts() async {
    final apdu = Uint8List.fromList([0x00, OATH_INS_LIST, 0x00, 0x00, 0x00]);
    final resp = await transport.sendApdu(apdu);
    return _parseAccountList(resp);
  }

  /// Calculate TOTP/HOTP code for an account
  Future<OathCode?> calculateCode(String accountName, {int? digits}) async {
    final nameBytes = utf8.encode(accountName);

    // Build challenge (current UTC time / 30 for TOTP)
    final timestamp = DateTime.now().toUtc().millisecondsSinceEpoch ~/ 1000;
    final challenge = (timestamp ~/ 30);
    final challengeBytes = Uint8List(8);
    final challengeData = ByteData.view(challengeBytes.buffer);
    challengeData.setUint64(0, challenge, Endian.big);

    final data = <int>[
      TAG_NAME,
      nameBytes.length,
      ...nameBytes,
      TAG_CHALLENGE,
      8,
      ...challengeBytes,
    ];

    final apdu = Uint8List.fromList(
        [0x00, OATH_INS_CALCULATE, 0x00, 0x01, data.length, ...data, 0x00]);

    final resp = await transport.sendApdu(apdu);

    if (_isSuccess(resp) && resp.length >= 7) {
      // Parse response: 0x76 len digits [4 bytes code]
      if (resp[0] == TAG_RESPONSE) {
        final respDigits = resp[2];
        final codeValue =
            (resp[3] << 24) | (resp[4] << 16) | (resp[5] << 8) | resp[6];
        final divisor = _getDivisor(respDigits);
        final code = (codeValue % divisor).toString().padLeft(respDigits, '0');
        return OathCode(code: code, digits: respDigits);
      }
    }
    return null;
  }

  /// Add a new OATH credential (PUT command)
  Future<bool> addOathCredential({
    required String name,
    required Uint8List secret,
    OathType type = OathType.totp,
    OathAlgorithm algorithm = OathAlgorithm.sha1,
    int digits = 6,
    bool touchRequired = false,
  }) async {
    final nameBytes = utf8.encode(name);

    // Build property byte: [type (4 bits)][algorithm (4 bits)]
    final typeByte = type == OathType.totp ? 0x20 : 0x10;
    final algByte = algorithm == OathAlgorithm.sha256 ? 0x02 : 0x01;
    final property = typeByte | algByte;

    final data = <int>[
      TAG_NAME,
      nameBytes.length,
      ...nameBytes,
      TAG_KEY,
      secret.length + 2,
      property,
      digits,
      ...secret,
    ];

    final apdu = Uint8List.fromList(
        [0x00, OATH_INS_PUT, 0x00, 0x00, data.length, ...data]);

    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  /// Delete an OATH credential
  Future<bool> deleteOathCredential(String name) async {
    final nameBytes = utf8.encode(name);
    final data = <int>[TAG_NAME, nameBytes.length, ...nameBytes];

    final apdu = Uint8List.fromList(
        [0x00, OATH_INS_DELETE, 0x00, 0x00, data.length, ...data]);

    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  /// Set/Change OATH PIN
  Future<bool> setOathPin(String pin) async {
    final pinBytes = Uint8List.fromList(utf8.encode(pin));
    final pinHash = Uint8List.fromList(sha256.convert(pinBytes).bytes);

    final data = <int>[TAG_KEY, pinHash.length, ...pinHash];
    final apdu = Uint8List.fromList(
        [0x00, OATH_INS_SET_CODE, 0x00, 0x00, data.length, ...data]);

    // Zeroize sensitive materials
    pinBytes.fillRange(0, pinBytes.length, 0);
    pinHash.fillRange(0, pinHash.length, 0);

    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  /// Validate OATH PIN
  Future<bool> validateOathPin(String pin) async {
    final pinBytes = Uint8List.fromList(utf8.encode(pin));
    final pinHash = Uint8List.fromList(sha256.convert(pinBytes).bytes);

    final data = <int>[TAG_RESPONSE, pinHash.length, ...pinHash];
    final apdu = Uint8List.fromList(
        [0x00, OATH_INS_VALIDATE, 0x00, 0x00, data.length, ...data]);

    // Zeroize sensitive materials
    pinBytes.fillRange(0, pinBytes.length, 0);
    pinHash.fillRange(0, pinHash.length, 0);

    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  /// Reset OATH applet (delete all credentials and PIN)
  Future<bool> resetOath() async {
    final apdu = Uint8List.fromList([0x00, OATH_INS_RESET, 0xDE, 0xAD, 0x00]);
    final resp = await transport.sendApdu(apdu);
    return _isSuccess(resp);
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // MANAGEMENT OPERATIONS (WebUSB)
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get firmware version from device
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

  /// Get device status (credential counts)
  Future<DeviceStatus?> getDeviceStatus() async {
    try {
      final resp =
          await transport.sendCtapHid(MGMT_CMD_GET_STATUS, Uint8List(0));
      if (resp.length >= 5 && resp[0] == 0x00) {
        return DeviceStatus(
          fido2Count: resp[1],
          fido2Max: resp[2],
          oathCount: resp[3],
          oathMax: resp[4],
        );
      }
    } catch (e) {
      return null;
    }
    return null;
  }

  /// Trigger reboot into BOOTSEL mode for firmware update
  Future<bool> enterBootloaderMode() async {
    try {
      final resp =
          await transport.sendCtapHid(MGMT_CMD_REBOOT_BOOTLOADER, Uint8List(0));
      return resp.isNotEmpty && resp[0] == 0x00;
    } catch (e) {
      return false;
    }
  }

  /// Reset entire device (all data)
  Future<bool> resetDevice() async {
    try {
      final resp =
          await transport.sendCtapHid(MGMT_CMD_RESET_DEVICE, Uint8List(0));
      return resp.isNotEmpty && resp[0] == 0x00;
    } catch (e) {
      return false;
    }
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // FIDO2 OPERATIONS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Get FIDO2 authenticator info (requires CBOR decoder)
  Future<Map<dynamic, dynamic>?> getFidoInfo() async {
    try {
      final resp =
          await transport.sendCtapHid(CTAP2_CMD_GET_INFO, Uint8List(0));
      if (resp.isNotEmpty && resp[0] == 0x00) {
        // Success: 0x00 followed by CBOR payload
        final cborPayload = resp.sublist(1);
        final decoded = cbor.decode(cborPayload);
        if (decoded is CborMap) {
          return decoded.toObject() as Map<dynamic, dynamic>;
        }
      }
    } catch (e) {
      print('Error getting FIDO info: $e');
    }
    return null;
  }

  /// List all FIDO2 Resident Keys (Requires CTAP 2.1 Credential Management)
  Future<List<FidoCredential>> listFidoCredentials() async {
    final List<FidoCredential> credentials = [];
    try {
      // 1. Get RP list
      final rpListReq = CborMap({
        CborSmallInt(0x01): CborSmallInt(CTAP2_SUB_GET_RP), // Sub-command
      });

      final rpResp = await transport.sendCtapHid(
          CTAP2_CMD_CRED_MGMT, Uint8List.fromList(cbor.encode(rpListReq)));
      if (rpResp.isEmpty || rpResp[0] != 0x00) return [];

      // ... simplified for brevity, in a real scenario we'd loop through RPs and then Credentials ...
      // For now, if getInfo showed we have credentials, we'd iterate.
      // This is a placeholder for the full multi-step CTAP2 flow.
    } catch (e) {
      print('Error listing FIDO credentials: $e');
    }
    return credentials;
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // HELPERS
  // ═══════════════════════════════════════════════════════════════════════════

  /// Check if response indicates success (SW1=0x90, SW2=0x00)
  /// Throws PinRequiredException if status is 0x6982
  bool _isSuccess(Uint8List resp) {
    if (resp.length < 2) return false;
    final sw1 = resp[resp.length - 2];
    final sw2 = resp[resp.length - 1];
    final sw = (sw1 << 8) | sw2;

    if (sw == 0x9000) return true;
    if (sw == 0x6982) throw PinRequiredException();

    return false;
  }

  /// Get divisor for TOTP code truncation
  int _getDivisor(int digits) {
    switch (digits) {
      case 6:
        return 1000000;
      case 7:
        return 10000000;
      case 8:
        return 100000000;
      default:
        return 1000000;
    }
  }

  /// Parse account list response
  List<Map<String, String>> _parseAccountList(Uint8List response) {
    final List<Map<String, String>> result = [];
    int i = 0;

    while (i < response.length - 2) {
      if (response[i] == TAG_NAME_LIST) {
        int len = response[i + 1];
        int j = i + 2;
        int end = j + len;
        String? name;
        String type = "TOTP";
        String algorithm = "SHA1";

        while (j < end && j < response.length - 1) {
          int tag = response[j];
          int tagLen = response[j + 1];

          if (tag == TAG_NAME && j + 2 + tagLen <= response.length) {
            name = utf8.decode(response.sublist(j + 2, j + 2 + tagLen));
          } else if (tag == TAG_PROPERTY && j + 2 < response.length) {
            final prop = response[j + 2];
            type = (prop & 0x20 != 0) ? "TOTP" : "HOTP";
            algorithm = (prop & 0x02 != 0) ? "SHA256" : "SHA1";
          }
          j += 2 + tagLen;
        }

        if (name != null) {
          result.add({
            "name": name,
            "type": type,
            "algorithm": algorithm,
          });
        }
        i += 2 + len;
      } else {
        i++;
      }
    }
    return result;
  }
}
