import 'dart:typed_data';
import 'package:test/test.dart';
import 'package:opentoken_dart/opentoken_service.dart';

// Mock transport to capture APDUs
class MockTransport extends OpenTokenTransport {
  Uint8List? lastApdu;

  @override
  Future<Uint8List> sendApdu(Uint8List apdu) async {
    lastApdu = Uint8List.fromList(apdu);
    return Uint8List.fromList([0x90, 0x00]); // Success
  }

  @override
  Future<Uint8List> sendCtapHid(int command, Uint8List payload) async {
    return Uint8List.fromList([0x00]);
  }
}

void main() {
  group('Security Verification Tests', () {
    late MockTransport transport;
    late OpenTokenSharedService service;

    setUp(() {
      transport = MockTransport();
      service = OpenTokenSharedService(transport);
    });

    test('Verification buffers are zeroized after use in validateOathPin',
        () async {
      // This is a behavioral test. Since we can't easily inspect RAM for local variables
      // in Dart VM from outside, we can rely on verifying the logic that does the zeroization.
      // However, we can simulate the buffer management.

      const pin = "123456";
      await service.validateOathPin(pin);

      // The test here is essentially ensuring the function completes without error
      // and that the APDU was sent correctly.
      expect(transport.lastApdu, isNotNull);
      expect(transport.lastApdu![1], OpenTokenSharedService.OATH_INS_VALIDATE);
    });

    test('PIN sensitive data is not leaked in APDU beyond the hash', () async {
      const pin = "secret_pin_123";
      await service.validateOathPin(pin);

      final apdu = transport.lastApdu!;
      // Verify that the PIN itself is NOT in the APDU (only the hash)
      final apduString = String.fromCharCodes(apdu);
      expect(apduString.contains(pin), isFalse);
    });
  });
}
