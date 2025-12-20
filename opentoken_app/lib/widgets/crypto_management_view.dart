import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class CryptoManagementView extends StatefulWidget {
  const CryptoManagementView({super.key});

  @override
  State<CryptoManagementView> createState() => _CryptoManagementViewState();
}

class _CryptoManagementViewState extends State<CryptoManagementView> {
  final List<Map<String, dynamic>> _keys = [
    {
      "slot": "#01",
      "algo": "ECDSA P-256",
      "fingerprint": "a1b2c3d4...98765432",
      "date": "Oct 27, 2023"
    },
    {
      "slot": "#02",
      "algo": "Ed25519",
      "fingerprint": "e5f6g7h8...12345678",
      "date": "Oct 28, 2023"
    },
    {
      "slot": "#03",
      "algo": "RSA-2048",
      "fingerprint": "i9j0k1l2...56789012",
      "date": "Nov 01, 2023"
    },
  ];

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(40.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Expanded(
            flex: 6,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  "Crypto Management",
                  style: GoogleFonts.inter(
                      fontSize: 32,
                      fontWeight: FontWeight.bold,
                      color: Colors.white),
                ),
                const SizedBox(height: 8),
                Text(
                  "Manage asymmetric keys stored on your hardware device. Sign challenges and generate new identities securely offline.",
                  style: GoogleFonts.inter(color: Colors.white54, fontSize: 14),
                ),
                const SizedBox(height: 48),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text("Stored Keys (3/16 slots used)",
                        style: GoogleFonts.inter(
                            color: Colors.white, fontWeight: FontWeight.w600)),
                    IconButton(
                        icon: const Icon(Icons.refresh,
                            color: Colors.white38, size: 20),
                        onPressed: () {}),
                  ],
                ),
                const SizedBox(height: 16),
                _buildKeyTable(),
                const SizedBox(height: 24),
                Center(
                  child: TextButton.icon(
                    onPressed: () {},
                    icon: const Icon(Icons.add, size: 18),
                    label: const Text("Generate New Key"),
                    style: TextButton.styleFrom(
                        foregroundColor: const Color(0xFF007BFF)),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(width: 48),
          Expanded(
            flex: 4,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildToolCard("Sign Challenge", [
                  _buildLabel("INPUT DATA"),
                  _buildInputArea("0x..."),
                  const SizedBox(height: 24),
                  _buildLabel("SIGNING KEY"),
                  _buildDropdown("Slot #01 (ECDSA P-256)"),
                  const SizedBox(height: 32),
                  _buildPrimaryButton("Sign with Device", Icons.fingerprint),
                  const SizedBox(height: 24),
                  _buildLabel("SIGNATURE OUTPUT"),
                  _buildInputArea("Waiting for signature...", readOnly: true),
                ]),
                const SizedBox(height: 32),
                _buildToolCard("Quick Actions", [
                  Text(
                    "Generate a new key pair on the next available slot (#04).",
                    style:
                        GoogleFonts.inter(color: Colors.white54, fontSize: 13),
                  ),
                  const SizedBox(height: 16),
                  Row(
                    children: [
                      Expanded(child: _buildDropdown("ECDSA P-256")),
                      const SizedBox(width: 12),
                      ElevatedButton(
                        onPressed: () {},
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.white.withOpacity(0.05),
                          foregroundColor: Colors.white,
                          shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(8)),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 16, vertical: 12),
                        ),
                        child: const Text("Generate"),
                      ),
                    ],
                  ),
                ]),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildKeyTable() {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.02),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Table(
        columnWidths: const {
          0: FlexColumnWidth(1),
          1: FlexColumnWidth(2),
          2: FlexColumnWidth(4),
          3: FlexColumnWidth(2),
          4: FlexColumnWidth(1),
        },
        children: [
          _buildTableHeader(),
          ..._keys.map((k) => _buildTableRow(k)),
        ],
      ),
    );
  }

  TableRow _buildTableHeader() {
    return TableRow(
      decoration: BoxDecoration(
          border: Border(
              bottom: BorderSide(color: Colors.white.withOpacity(0.05)))),
      children:
          ["SLOT", "ALGORITHM", "FINGERPRINT (SHA-256)", "CREATED", "ACTIONS"]
              .map((t) => Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Text(t,
                        style: GoogleFonts.inter(
                            color: Colors.white38,
                            fontSize: 10,
                            fontWeight: FontWeight.bold,
                            letterSpacing: 1)),
                  ))
              .toList(),
    );
  }

  TableRow _buildTableRow(Map<String, dynamic> key) {
    return TableRow(
      children: [
        _buildTableCell(key["slot"], Colors.white38),
        _buildTableCell(key["algo"], const Color(0xFFBC00FF), isBadge: true),
        _buildTableCell(key["fingerprint"], Colors.white70),
        _buildTableCell(key["date"], Colors.white38),
        Padding(
          padding: const EdgeInsets.all(8.0),
          child: IconButton(
              icon: const Icon(Icons.delete_outline,
                  color: Colors.white24, size: 18),
              onPressed: () {}),
        ),
      ],
    );
  }

  Widget _buildTableCell(String text, Color color, {bool isBadge = false}) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 20.0),
      child: isBadge
          ? Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              decoration: BoxDecoration(
                  color: color.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(4),
                  border: Border.all(color: color.withOpacity(0.2))),
              child: Text(text,
                  style: GoogleFonts.inter(
                      color: color, fontSize: 10, fontWeight: FontWeight.bold)),
            )
          : Text(text, style: GoogleFonts.inter(color: color, fontSize: 12)),
    );
  }

  Widget _buildToolCard(String title, List<Widget> children) {
    return Container(
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(title,
              style: GoogleFonts.inter(
                  color: Colors.white,
                  fontSize: 18,
                  fontWeight: FontWeight.bold)),
          const SizedBox(height: 24),
          ...children,
        ],
      ),
    );
  }

  Widget _buildLabel(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(text,
          style: GoogleFonts.inter(
              color: Colors.white38,
              fontSize: 10,
              fontWeight: FontWeight.bold,
              letterSpacing: 1.2)),
    );
  }

  Widget _buildInputArea(String hint, {bool readOnly = false}) {
    return Container(
      height: 100,
      decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.3),
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: Colors.white.withOpacity(0.1))),
      padding: const EdgeInsets.all(12),
      child: TextField(
        readOnly: readOnly,
        maxLines: null,
        style: GoogleFonts.jetBrainsMono(color: Colors.white70, fontSize: 12),
        decoration: InputDecoration(
            hintText: hint,
            hintStyle: GoogleFonts.jetBrainsMono(color: Colors.white12),
            border: InputBorder.none),
      ),
    );
  }

  Widget _buildDropdown(String text) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: Colors.white.withOpacity(0.1))),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(text,
              style: GoogleFonts.inter(color: Colors.white70, fontSize: 13)),
          const Icon(Icons.keyboard_arrow_down,
              color: Colors.white38, size: 20),
        ],
      ),
    );
  }

  Widget _buildPrimaryButton(String label, IconData icon) {
    return SizedBox(
      width: double.infinity,
      child: ElevatedButton.icon(
        onPressed: () {},
        icon: Icon(icon, size: 18),
        label: Text(label),
        style: ElevatedButton.styleFrom(
          backgroundColor: const Color(0xFF007BFF),
          foregroundColor: Colors.white,
          padding: const EdgeInsets.symmetric(vertical: 20),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
        ),
      ),
    );
  }
}
