import 'dart:convert';
import 'dart:typed_data';

/// Shared OATH Service for OpenToken (Cross-Platform)
/// This class contains the PURE LOGIC for the OATH protocol.
/// It creates APDUs and parses status words regardless of the underlying transport (NFC/USB).
class OpenTokenSharedService {
  static const List<int> OATH_AID = [0xA0, 0x00, 0x00, 0x05, 0x27, 0x21, 0x01, 0x01];

  /// --- APDU Factory Methods ---

  /// Builds the SELECT applet command
  Uint8List selectApplet() {
    return Uint8List.fromList([
      0x00, 0xA4, 0x04, 0x00, 
      OATH_AID.length, 
      ...OATH_AID
    ]);
  }

  /// Builds the LIST accounts command
  Uint8List listAccounts() {
    return Uint8List.fromList([0x00, 0xA1, 0x00, 0x00, 0x00]);
  }

  /// Builds the CALCULATE code command for a specific account
  Uint8List calculateCode(String accountName) {
    final nameBytes = utf8.encode(accountName);
    // Format: CLA INS P1 P2 Lc [Tag 0x71, Len, NameData] [Le 0x00]
    return Uint8List.fromList([
      0x00, 0xA2, 0x00, 0x01, 
      nameBytes.length + 2, 
      0x71, nameBytes.length, 
      ...nameBytes, 
      0x00
    ]);
  }

  /// Builds the ADD (PUT) account command
  Uint8List addAccount(String name, List<int> secretKey, {bool isTotp = true}) {
    final nameBytes = utf8.encode(name);
    final property = isTotp ? 0x21 : 0x11; // TOTP-SHA1 vs HOTP-SHA1

    final data = [
      0x71, nameBytes.length, ...nameBytes,     // Tag 0x71: Name
      0x73, secretKey.length, ...secretKey,     // Tag 0x73: Key
      0x75, 0x01, property                      // Tag 0x75: Property
    ];

    return Uint8List.fromList([
      0x00, 0x01, 0x00, 0x00, 
      data.length,
      ...data
    ]);
  }

  /// --- Response Parsing Logic ---

  /// Simplistic TLV parser for account lists
  List<Map<String, String>> parseAccountList(Uint8List response) {
    final List<Map<String, String>> result = [];
    int i = 0;
    while (i < response.length - 2) { // Subtract 2 for SW1 SW2
      if (response[i] == 0x72) { // Name List Tag
        int len = response[i + 1];
        int start = i + 2;
        int end = start + len;
        
        // Parse inner TLV (71 Name, 75 Prop)
        String? name;
        String type = "TOTP";
        
        int j = start;
        while (j < end) {
            int tag = response[j];
            int tagLen = response[j+1];
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
