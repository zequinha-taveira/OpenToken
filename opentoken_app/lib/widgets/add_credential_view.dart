import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme.dart';

class AddCredentialView extends StatefulWidget {
  final VoidCallback onCancel;
  final Function(String name, String secret, String algo, int digits) onSave;

  const AddCredentialView({
    super.key,
    required this.onCancel,
    required this.onSave,
  });

  @override
  State<AddCredentialView> createState() => _AddCredentialViewState();
}

class _AddCredentialViewState extends State<AddCredentialView> {
  final _nameController = TextEditingController();
  final _secretController = TextEditingController();
  String _algorithm = "SHA-1";
  int _digits = 6;
  bool _showSecret = false;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: OpenTokenTheme.deepBackground,
      padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 32),
      child: SingleChildScrollView(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                IconButton(
                  icon: const Icon(Icons.arrow_back, color: Colors.white70),
                  onPressed: widget.onCancel,
                ),
                const SizedBox(width: 8),
                Text(
                  "Add New Credential",
                  style: GoogleFonts.inter(
                    fontSize: 28,
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Padding(
              padding: const EdgeInsets.only(left: 48),
              child: Text(
                "Configure the parameters for your TOTP entry manually.",
                style: GoogleFonts.inter(color: Colors.white54, fontSize: 14),
              ),
            ),
            const SizedBox(height: 48),
            _buildLabel("NAME"),
            _buildTextField(
              controller: _nameController,
              hint: "e.g. GitHub, AWS, ProtonMail",
            ),
            const SizedBox(height: 24),
            _buildLabel("SECRET (BASE32)"),
            _buildTextField(
              controller: _secretController,
              hint: "JBSWY3DPEHPK3PXP",
              isPassword: !_showSecret,
              suffix: IconButton(
                icon: Icon(
                  _showSecret ? Icons.visibility_off : Icons.visibility,
                  color: Colors.white38,
                  size: 20,
                ),
                onPressed: () => setState(() => _showSecret = !_showSecret),
              ),
            ),
            const SizedBox(height: 24),
            Row(
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _buildLabel("ALGORITHM"),
                      _buildToggle(["SHA-1", "SHA-256"], _algorithm,
                          (val) => setState(() => _algorithm = val)),
                    ],
                  ),
                ),
                const SizedBox(width: 24),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _buildLabel("DIGITS"),
                      _buildToggle(["6", "8"], _digits.toString(),
                          (val) => setState(() => _digits = int.parse(val))),
                    ],
                  ),
                ),
              ],
            ),
            const SizedBox(height: 40),
            _buildSecurityWarning(),
            const SizedBox(height: 48),
            const Divider(color: Colors.white10),
            const SizedBox(height: 24),
            Row(
              mainAxisAlignment: MainAxisAlignment.end,
              children: [
                TextButton(
                  onPressed: widget.onCancel,
                  child: Text("Cancel",
                      style: GoogleFonts.inter(color: Colors.white54)),
                ),
                const SizedBox(width: 24),
                ElevatedButton.icon(
                  onPressed: () => widget.onSave(_nameController.text,
                      _secretController.text, _algorithm, _digits),
                  icon: const Icon(Icons.save, size: 18),
                  label: const Text("Write to Device"),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF007BFF),
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(
                        horizontal: 24, vertical: 20),
                    shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(8)),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildLabel(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(
        text,
        style: GoogleFonts.inter(
          fontSize: 11,
          fontWeight: FontWeight.bold,
          color: Colors.white38,
          letterSpacing: 1.2,
        ),
      ),
    );
  }

  Widget _buildTextField({
    required TextEditingController controller,
    required String hint,
    bool isPassword = false,
    Widget? suffix,
  }) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: TextField(
        controller: controller,
        obscureText: isPassword,
        style: GoogleFonts.inter(color: Colors.white),
        decoration: InputDecoration(
          hintText: hint,
          hintStyle: GoogleFonts.inter(color: Colors.white12),
          contentPadding:
              const EdgeInsets.symmetric(horizontal: 16, vertical: 16),
          border: InputBorder.none,
          suffixIcon: suffix,
        ),
      ),
    );
  }

  Widget _buildToggle(
      List<String> options, String current, Function(String) onSelect) {
    return Container(
      height: 48,
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: Row(
        children: options.map((opt) {
          final isSelected = opt == current;
          return Expanded(
            child: GestureDetector(
              onTap: () => onSelect(opt),
              child: Container(
                margin: const EdgeInsets.all(4),
                decoration: BoxDecoration(
                  color:
                      isSelected ? const Color(0xFF007BFF) : Colors.transparent,
                  borderRadius: BorderRadius.circular(6),
                ),
                alignment: Alignment.center,
                child: Text(
                  opt,
                  style: GoogleFonts.inter(
                    color: isSelected ? Colors.white : Colors.white38,
                    fontSize: 12,
                    fontWeight:
                        isSelected ? FontWeight.bold : FontWeight.normal,
                  ),
                ),
              ),
            ),
          );
        }).toList(),
      ),
    );
  }

  Widget _buildSecurityWarning() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF007BFF).withOpacity(0.05),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: const Color(0xFF007BFF).withOpacity(0.2)),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Icon(Icons.lock_person_outlined,
              color: Color(0xFF007BFF), size: 24),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  "Device-Only Storage",
                  style: GoogleFonts.inter(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 15,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  "This secret will be stored ONLY on the connected hardware device. It is never transmitted over any network or stored on this computer.",
                  style: GoogleFonts.inter(
                    color: Colors.white54,
                    fontSize: 13,
                    height: 1.5,
                  ),
                ),
                const SizedBox(height: 12),
                Text(
                  "⚠️ NO BACKUP: If you lose this device, you lose access to this credential.",
                  style: GoogleFonts.inter(
                    color: const Color(0xFF00F0FF).withOpacity(0.8),
                    fontSize: 12,
                    fontWeight: FontWeight.w500,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
