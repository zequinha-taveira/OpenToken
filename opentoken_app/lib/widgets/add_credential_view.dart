import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:base32/base32.dart';
import 'package:opentoken_dart/opentoken_service.dart';
import '../theme.dart';

/// Add Credential View - PRD Sections 3.2 & 3.3
/// Input method selection followed by manual entry form
class AddCredentialView extends StatefulWidget {
  final OpenTokenSharedService service;
  final VoidCallback onCancel;
  final VoidCallback onSuccess;

  const AddCredentialView({
    super.key,
    required this.service,
    required this.onCancel,
    required this.onSuccess,
  });

  @override
  State<AddCredentialView> createState() => _AddCredentialViewState();
}

class _AddCredentialViewState extends State<AddCredentialView> {
  // View state
  bool _showManualEntry = false;

  // Form state
  final _nameController = TextEditingController();
  final _secretController = TextEditingController();
  final _periodController = TextEditingController(text: '30');
  String _type = 'TOTP';
  String _algorithm = 'SHA1';
  int _digits = 6;
  bool _touchRequired = false;
  bool _showSecret = false;
  bool _isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: OpenTokenTheme.deepBackground,
      child:
          _showManualEntry ? _buildManualEntryForm() : _buildMethodSelection(),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // INPUT METHOD SELECTION - PRD Section 3.2
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildMethodSelection() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(OpenTokenTheme.space7),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header
          Row(
            children: [
              IconButton(
                icon: const Icon(Icons.arrow_back, color: Colors.white70),
                onPressed: widget.onCancel,
              ),
              const SizedBox(width: OpenTokenTheme.space2),
              Text(
                "Add Credential",
                style: GoogleFonts.inter(
                  fontSize: OpenTokenTheme.text2Xl,
                  fontWeight: OpenTokenTheme.fontBold,
                  color: Colors.white,
                ),
              ),
            ],
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          Text(
            "Choose input method:",
            style: GoogleFonts.inter(
              color: Colors.white54,
              fontSize: OpenTokenTheme.textBase,
            ),
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // QR Code option
          _buildMethodCard(
            icon: Icons.qr_code_scanner,
            title: "Scan QR Code",
            description: "Scan the QR code shown by the service",
            onTap: () {
              // TODO: Navigate to QR Scanner
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('QR Scanner coming soon')),
              );
            },
          ),

          const SizedBox(height: OpenTokenTheme.space4),

          // Manual Entry option
          _buildMethodCard(
            icon: Icons.edit,
            title: "Manual Entry",
            description: "Enter the secret key manually",
            onTap: () => setState(() => _showManualEntry = true),
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // Divider with "OR"
          Row(
            children: [
              Expanded(child: Divider(color: Colors.white.withOpacity(0.1))),
              Padding(
                padding: const EdgeInsets.symmetric(
                    horizontal: OpenTokenTheme.space4),
                child: Text(
                  "OR",
                  style: GoogleFonts.inter(
                    color: Colors.white24,
                    fontSize: OpenTokenTheme.textSm,
                    fontWeight: OpenTokenTheme.fontMedium,
                  ),
                ),
              ),
              Expanded(child: Divider(color: Colors.white.withOpacity(0.1))),
            ],
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          Text(
            "Quick setup for popular services:",
            style: GoogleFonts.inter(
              color: Colors.white54,
              fontSize: OpenTokenTheme.textSm,
            ),
          ),

          const SizedBox(height: OpenTokenTheme.space4),

          // Quick setup buttons
          Wrap(
            spacing: OpenTokenTheme.space3,
            runSpacing: OpenTokenTheme.space3,
            children: [
              _buildQuickSetupButton("GitHub", "🐙"),
              _buildQuickSetupButton("Gmail", "📧"),
              _buildQuickSetupButton("AWS", "☁️"),
              _buildQuickSetupButton("Discord", "💬"),
              _buildQuickSetupButton("Microsoft", "🪟"),
              _buildQuickSetupButton("Dropbox", "📦"),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildMethodCard({
    required IconData icon,
    required String title,
    required String description,
    required VoidCallback onTap,
  }) {
    return Container(
      decoration: BoxDecoration(
        gradient: OpenTokenTheme.cardGradient,
        borderRadius: OpenTokenTheme.borderRadiusXl,
        border: OpenTokenTheme.cardBorder,
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: OpenTokenTheme.borderRadiusXl,
          child: Padding(
            padding: const EdgeInsets.all(OpenTokenTheme.space5),
            child: Row(
              children: [
                Container(
                  width: 56,
                  height: 56,
                  decoration: BoxDecoration(
                    color: OpenTokenTheme.primary.withOpacity(0.1),
                    borderRadius: OpenTokenTheme.borderRadiusLg,
                  ),
                  child: Icon(
                    icon,
                    color: OpenTokenTheme.primary,
                    size: 28,
                  ),
                ),
                const SizedBox(width: OpenTokenTheme.space4),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        title,
                        style: GoogleFonts.inter(
                          color: Colors.white,
                          fontSize: OpenTokenTheme.textLg,
                          fontWeight: OpenTokenTheme.fontSemibold,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        description,
                        style: GoogleFonts.inter(
                          color: Colors.white54,
                          fontSize: OpenTokenTheme.textSm,
                        ),
                      ),
                    ],
                  ),
                ),
                const Icon(
                  Icons.chevron_right,
                  color: Colors.white24,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildQuickSetupButton(String name, String emoji) {
    return InkWell(
      onTap: () {
        _nameController.text = name;
        setState(() => _showManualEntry = true);
      },
      borderRadius: OpenTokenTheme.borderRadiusMd,
      child: Container(
        padding: const EdgeInsets.symmetric(
          horizontal: OpenTokenTheme.space4,
          vertical: OpenTokenTheme.space3,
        ),
        decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.05),
          borderRadius: OpenTokenTheme.borderRadiusMd,
          border: Border.all(color: Colors.white.withOpacity(0.1)),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(emoji, style: const TextStyle(fontSize: 18)),
            const SizedBox(width: OpenTokenTheme.space2),
            Text(
              name,
              style: GoogleFonts.inter(
                color: Colors.white70,
                fontSize: OpenTokenTheme.textSm,
              ),
            ),
          ],
        ),
      ),
    );
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // MANUAL ENTRY FORM - PRD Section 3.3
  // ═══════════════════════════════════════════════════════════════════════════

  Widget _buildManualEntryForm() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(OpenTokenTheme.space7),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header
          Row(
            children: [
              IconButton(
                icon: const Icon(Icons.arrow_back, color: Colors.white70),
                onPressed: () => setState(() => _showManualEntry = false),
              ),
              const SizedBox(width: OpenTokenTheme.space2),
              Text(
                "Manual Entry",
                style: GoogleFonts.inter(
                  fontSize: OpenTokenTheme.text2Xl,
                  fontWeight: OpenTokenTheme.fontBold,
                  color: Colors.white,
                ),
              ),
              const Spacer(),
              IconButton(
                icon: const Icon(Icons.help_outline, color: Colors.white38),
                onPressed: () {},
              ),
            ],
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // Service Name
          _buildLabel("Service Name *"),
          _buildTextField(
            controller: _nameController,
            hint: "e.g. GitHub, AWS, ProtonMail",
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Secret Key
          _buildLabel("Secret Key (Base32) *"),
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
          const SizedBox(height: OpenTokenTheme.space2),
          Text(
            'ℹ️ Usually provided as "secret key" or "setup key"',
            style: GoogleFonts.inter(
              color: Colors.white38,
              fontSize: OpenTokenTheme.textXs,
            ),
          ),

          const SizedBox(height: OpenTokenTheme.space6),

          // Divider
          Row(
            children: [
              Expanded(child: Divider(color: Colors.white.withOpacity(0.1))),
              Padding(
                padding: const EdgeInsets.symmetric(
                    horizontal: OpenTokenTheme.space4),
                child: Text(
                  "Advanced Options (Optional)",
                  style: GoogleFonts.inter(
                    color: Colors.white38,
                    fontSize: OpenTokenTheme.textXs,
                  ),
                ),
              ),
              Expanded(child: Divider(color: Colors.white.withOpacity(0.1))),
            ],
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Type selection
          _buildLabel("Type"),
          _buildToggleButtons(
            options: ['TOTP', 'HOTP'],
            selected: _type,
            labels: ['TOTP (Time-based)', 'HOTP (Counter-based)'],
            onSelect: (val) => setState(() => _type = val),
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Algorithm selection
          _buildLabel("Algorithm"),
          _buildToggleButtons(
            options: ['SHA1', 'SHA256', 'SHA512'],
            selected: _algorithm,
            onSelect: (val) => setState(() => _algorithm = val),
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Digits selection
          _buildLabel("Digits"),
          _buildToggleButtons(
            options: ['6', '7', '8'],
            selected: _digits.toString(),
            onSelect: (val) => setState(() => _digits = int.parse(val)),
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Period input (TOTP only)
          if (_type == 'TOTP') ...[
            _buildLabel("Period (seconds)"),
            _buildTextField(
              controller: _periodController,
              hint: "30",
              keyboardType: TextInputType.number,
            ),
            const SizedBox(height: OpenTokenTheme.space5),
          ],

          // Touch required checkbox
          Container(
            padding: const EdgeInsets.all(OpenTokenTheme.space4),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.02),
              borderRadius: OpenTokenTheme.borderRadiusMd,
              border: Border.all(color: Colors.white.withOpacity(0.05)),
            ),
            child: Row(
              children: [
                Checkbox(
                  value: _touchRequired,
                  onChanged: (val) =>
                      setState(() => _touchRequired = val ?? false),
                  activeColor: OpenTokenTheme.primary,
                ),
                const SizedBox(width: OpenTokenTheme.space2),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        "Require touch to generate code",
                        style: GoogleFonts.inter(
                          color: Colors.white,
                          fontSize: OpenTokenTheme.textSm,
                        ),
                      ),
                      Text(
                        "Physical confirmation required for each code",
                        style: GoogleFonts.inter(
                          color: Colors.white38,
                          fontSize: OpenTokenTheme.textXs,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: OpenTokenTheme.space6),

          // Security warning
          _buildSecurityWarning(),

          const SizedBox(height: OpenTokenTheme.space6),

          // Action buttons
          Row(
            mainAxisAlignment: MainAxisAlignment.end,
            children: [
              TextButton(
                onPressed: widget.onCancel,
                child: Text(
                  "Cancel",
                  style: GoogleFonts.inter(color: Colors.white54),
                ),
              ),
              const SizedBox(width: OpenTokenTheme.space4),
              ElevatedButton.icon(
                onPressed: _isLoading ? null : _handleSave,
                icon: _isLoading
                    ? const SizedBox(
                        width: 18,
                        height: 18,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          color: Colors.white,
                        ),
                      )
                    : const Icon(Icons.save, size: 18),
                label: Text(_isLoading ? "Adding..." : "Add Credential"),
                style: ElevatedButton.styleFrom(
                  backgroundColor: OpenTokenTheme.primary,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(
                    horizontal: OpenTokenTheme.space5,
                    vertical: OpenTokenTheme.space4,
                  ),
                  shape: RoundedRectangleBorder(
                    borderRadius: OpenTokenTheme.borderRadiusMd,
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildLabel(String text) {
    return Padding(
      padding: const EdgeInsets.only(bottom: OpenTokenTheme.space2),
      child: Text(
        text,
        style: GoogleFonts.inter(
          fontSize: OpenTokenTheme.textXs,
          fontWeight: OpenTokenTheme.fontSemibold,
          color: Colors.white54,
          letterSpacing: 0.5,
        ),
      ),
    );
  }

  Widget _buildTextField({
    required TextEditingController controller,
    required String hint,
    bool isPassword = false,
    Widget? suffix,
    TextInputType? keyboardType,
  }) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: OpenTokenTheme.borderRadiusMd,
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: TextField(
        controller: controller,
        obscureText: isPassword,
        keyboardType: keyboardType,
        style: GoogleFonts.inter(color: Colors.white),
        decoration: InputDecoration(
          hintText: hint,
          hintStyle: GoogleFonts.inter(color: Colors.white24),
          contentPadding: const EdgeInsets.symmetric(
            horizontal: OpenTokenTheme.space4,
            vertical: OpenTokenTheme.space4,
          ),
          border: InputBorder.none,
          suffixIcon: suffix,
        ),
      ),
    );
  }

  Widget _buildToggleButtons({
    required List<String> options,
    required String selected,
    List<String>? labels,
    required Function(String) onSelect,
  }) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: OpenTokenTheme.borderRadiusMd,
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: Row(
        children: options.asMap().entries.map((entry) {
          final idx = entry.key;
          final opt = entry.value;
          final isSelected = opt == selected;
          final label =
              labels != null && idx < labels.length ? labels[idx] : opt;

          return Expanded(
            child: GestureDetector(
              onTap: () => onSelect(opt),
              child: Container(
                padding:
                    const EdgeInsets.symmetric(vertical: OpenTokenTheme.space3),
                margin: const EdgeInsets.all(4),
                decoration: BoxDecoration(
                  color: isSelected
                      ? OpenTokenTheme.primary.withOpacity(0.2)
                      : Colors.transparent,
                  borderRadius: BorderRadius.circular(6),
                ),
                alignment: Alignment.center,
                child: Text(
                  label,
                  style: GoogleFonts.inter(
                    color: isSelected ? OpenTokenTheme.primary : Colors.white54,
                    fontSize: OpenTokenTheme.textSm,
                    fontWeight: isSelected
                        ? OpenTokenTheme.fontSemibold
                        : OpenTokenTheme.fontNormal,
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
      padding: const EdgeInsets.all(OpenTokenTheme.space4),
      decoration: BoxDecoration(
        color: OpenTokenTheme.primary.withOpacity(0.05),
        borderRadius: OpenTokenTheme.borderRadiusLg,
        border: Border.all(color: OpenTokenTheme.primary.withOpacity(0.2)),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Icon(
            Icons.lock_person_outlined,
            color: OpenTokenTheme.primary,
            size: 24,
          ),
          const SizedBox(width: OpenTokenTheme.space4),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  "Device-Only Storage",
                  style: GoogleFonts.inter(
                    color: Colors.white,
                    fontWeight: OpenTokenTheme.fontSemibold,
                    fontSize: OpenTokenTheme.textBase,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  "This secret will be stored ONLY on the connected hardware device. "
                  "It is never transmitted over any network or stored on this computer.",
                  style: GoogleFonts.inter(
                    color: Colors.white54,
                    fontSize: OpenTokenTheme.textSm,
                    height: 1.5,
                  ),
                ),
                const SizedBox(height: OpenTokenTheme.space3),
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: OpenTokenTheme.space3,
                    vertical: OpenTokenTheme.space2,
                  ),
                  decoration: BoxDecoration(
                    color: OpenTokenTheme.warning.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(4),
                  ),
                  child: Text(
                    "⚠️ NO BACKUP: If you lose this device, you lose access to this credential.",
                    style: GoogleFonts.inter(
                      color: OpenTokenTheme.warning,
                      fontSize: OpenTokenTheme.textXs,
                      fontWeight: OpenTokenTheme.fontMedium,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  void _handleSave() async {
    if (_nameController.text.isEmpty || _secretController.text.isEmpty) {
      _showError('Please fill in all required fields');
      return;
    }

    setState(() => _isLoading = true);

    try {
      // Clean and Validate Secret (Base32)
      final secretText =
          _secretController.text.replaceAll(' ', '').toUpperCase();
      Uint8List secretBytes;
      try {
        secretBytes = base32.decode(secretText);
      } catch (e) {
        _showError('Invalid Base32 secret key');
        setState(() => _isLoading = false);
        return;
      }

      // Determine Algorithm
      OathAlgorithm alg = OathAlgorithm.sha1;
      if (_algorithm == 'SHA256') alg = OathAlgorithm.sha256;
      if (_algorithm == 'SHA512') alg = OathAlgorithm.sha512;

      // Determine Type
      OathType type = _type == 'TOTP' ? OathType.totp : OathType.hotp;

      // Call Service
      final success = await widget.service.addOathCredential(
        name: _nameController.text,
        secret: secretBytes,
        type: type,
        algorithm: alg,
        digits: _digits,
        touchRequired: _touchRequired,
      );

      if (success) {
        widget.onSuccess();
      } else {
        _showError('Failed to write to device. Check connection.');
      }
    } catch (e) {
      _showError('Error: $e');
    } finally {
      if (mounted) {
        setState(() => _isLoading = false);
      }
    }
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: OpenTokenTheme.error,
      ),
    );
  }
}
