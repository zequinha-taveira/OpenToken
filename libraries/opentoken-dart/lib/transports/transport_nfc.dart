import 'dart:typed_data';
import 'package:flutter_nfc_kit/flutter_nfc_kit.dart';
import '../opentoken_service.dart';

/// Implementation of OpenTokenTransport for Mobile NFC devices.
class NfcTransport implements OpenTokenTransport {
  @override
  Future<Uint8List> sendApdu(Uint8List apdu) async {
    try {
      final response = await FlutterNfcKit.transceive(apdu);
      return response;
    } catch (e) {
      throw Exception("NFC Transceive Error: $e");
    }
  }

  @override
  Future<Uint8List> sendCtapHid(int command, Uint8List payload) async {
    // CTAPHID is strictly for USB. Over NFC, CTAP2 commands are encapsulated
    // in APDUs (ISO7816). We'll implement this mapping later if needed for mobile.
    throw UnimplementedError(
        "CTAPHID is NOT supported over NFC. Use APDU encapsulation.");
  }
}
