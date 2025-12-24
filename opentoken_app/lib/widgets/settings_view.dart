import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme.dart';

/// Settings View - PRD Section 3.5
/// Provides configuration for Device, Security, Application, and About sections
class SettingsView extends StatefulWidget {
  const SettingsView({super.key});

  @override
  State<SettingsView> createState() => _SettingsViewState();
}

class _SettingsViewState extends State<SettingsView> {
  // Application settings state
  String _theme = 'Dark';
  bool _autoCopy = true;
  bool _showNotifications = true;
  bool _minimizeToTray = false;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(OpenTokenTheme.space7),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header
          Text(
            "Settings",
            style: GoogleFonts.inter(
              fontSize: OpenTokenTheme.text2Xl,
              fontWeight: OpenTokenTheme.fontBold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: OpenTokenTheme.space2),
          Text(
            "Configure device security and application preferences.",
            style: GoogleFonts.inter(
              color: Colors.white54,
              fontSize: OpenTokenTheme.textSm,
            ),
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // ═══════════════════════════════════════════════════════════════════
          // DEVICE SECTION
          // ═══════════════════════════════════════════════════════════════════
          const _SectionHeader(title: "Device"),
          const SizedBox(height: OpenTokenTheme.space4),

          _NavigationTile(
            title: "Device Information",
            subtitle: "View hardware details and connection status",
            icon: Icons.info_outline,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Firmware Update",
            subtitle: "Check for and install firmware updates",
            icon: Icons.system_update,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Factory Reset",
            subtitle: "Erase all data and restore defaults",
            icon: Icons.restore,
            iconColor: OpenTokenTheme.error,
            onTap: () => _showFactoryResetDialog(),
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // ═══════════════════════════════════════════════════════════════════
          // SECURITY SECTION
          // ═══════════════════════════════════════════════════════════════════
          const _SectionHeader(title: "Security"),
          const SizedBox(height: OpenTokenTheme.space4),

          _NavigationTile(
            title: "Set PIN",
            subtitle: "Protect your device with a PIN",
            icon: Icons.pin,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Change PIN",
            subtitle: "Update your device PIN",
            icon: Icons.password,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Touch Policy",
            subtitle: "Configure when physical touch is required",
            icon: Icons.touch_app,
            onTap: () {},
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // ═══════════════════════════════════════════════════════════════════
          // APPLICATION SECTION
          // ═══════════════════════════════════════════════════════════════════
          const _SectionHeader(title: "Application"),
          const SizedBox(height: OpenTokenTheme.space4),

          _SettingsTile(
            title: "Theme",
            subtitle: "Choose your preferred visual appearance",
            child: _buildThemeSelector(),
          ),
          _ToggleTile(
            title: "Auto-copy codes",
            subtitle: "Automatically copy codes to clipboard",
            value: _autoCopy,
            onChanged: (val) => setState(() => _autoCopy = val),
          ),
          _ToggleTile(
            title: "Show notifications",
            subtitle: "Display system notifications",
            value: _showNotifications,
            onChanged: (val) => setState(() => _showNotifications = val),
          ),
          _ToggleTile(
            title: "Minimize to tray",
            subtitle: "Keep running in system tray when closed",
            value: _minimizeToTray,
            onChanged: (val) => setState(() => _minimizeToTray = val),
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // ═══════════════════════════════════════════════════════════════════
          // ABOUT SECTION
          // ═══════════════════════════════════════════════════════════════════
          const _SectionHeader(title: "About"),
          const SizedBox(height: OpenTokenTheme.space4),

          _buildAboutCard(),

          const SizedBox(height: OpenTokenTheme.space4),

          _NavigationTile(
            title: "Documentation",
            subtitle: "View guides and API reference",
            icon: Icons.menu_book,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Report Bug",
            subtitle: "Submit an issue on GitHub",
            icon: Icons.bug_report,
            onTap: () {},
          ),
          _NavigationTile(
            title: "Feature Request",
            subtitle: "Suggest new features",
            icon: Icons.lightbulb_outline,
            onTap: () {},
          ),

          const SizedBox(height: OpenTokenTheme.space7),

          // Footer
          _buildFooter(),
        ],
      ),
    );
  }

  Widget _buildThemeSelector() {
    return Container(
      padding: const EdgeInsets.all(4),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: OpenTokenTheme.borderRadiusMd,
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: ['Light', 'Dark', 'Auto'].map((option) {
          final isSelected = option == _theme;
          return GestureDetector(
            onTap: () => setState(() => _theme = option),
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              decoration: BoxDecoration(
                color: isSelected
                    ? OpenTokenTheme.primary.withOpacity(0.2)
                    : Colors.transparent,
                borderRadius: BorderRadius.circular(6),
              ),
              child: Text(
                option,
                style: GoogleFonts.inter(
                  color: isSelected ? OpenTokenTheme.primary : Colors.white54,
                  fontSize: OpenTokenTheme.textSm,
                  fontWeight: isSelected
                      ? OpenTokenTheme.fontSemibold
                      : OpenTokenTheme.fontNormal,
                ),
              ),
            ),
          );
        }).toList(),
      ),
    );
  }

  Widget _buildAboutCard() {
    return Container(
      padding: const EdgeInsets.all(OpenTokenTheme.space5),
      decoration: BoxDecoration(
        gradient: OpenTokenTheme.cardGradient,
        borderRadius: OpenTokenTheme.borderRadiusLg,
        border: OpenTokenTheme.cardBorder,
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  gradient: OpenTokenTheme.brandGradient,
                  borderRadius: OpenTokenTheme.borderRadiusMd,
                ),
                child: const Icon(Icons.shield, color: Colors.white, size: 24),
              ),
              const SizedBox(width: OpenTokenTheme.space4),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      "OpenToken Authenticator",
                      style: GoogleFonts.inter(
                        color: Colors.white,
                        fontWeight: OpenTokenTheme.fontBold,
                        fontSize: OpenTokenTheme.textLg,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      "Security Without Chains",
                      style: GoogleFonts.inter(
                        color: Colors.white54,
                        fontSize: OpenTokenTheme.textSm,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: OpenTokenTheme.space5),
          const Divider(color: Colors.white10),
          const SizedBox(height: OpenTokenTheme.space4),
          _buildInfoRow("App Version", "1.0.0"),
          _buildInfoRow("Protocol Version", "1.0"),
          _buildInfoRow("License", "MIT"),
          _buildInfoRow("Website", "opentoken.dev"),
          _buildInfoRow("GitHub", "github.com/opentoken"),
        ],
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: GoogleFonts.inter(
              color: Colors.white54,
              fontSize: OpenTokenTheme.textSm,
            ),
          ),
          Text(
            value,
            style: GoogleFonts.inter(
              color: Colors.white,
              fontSize: OpenTokenTheme.textSm,
              fontWeight: OpenTokenTheme.fontMedium,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildFooter() {
    return Container(
      padding: const EdgeInsets.symmetric(
        horizontal: OpenTokenTheme.space4,
        vertical: OpenTokenTheme.space3,
      ),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.02),
        borderRadius: OpenTokenTheme.borderRadiusLg,
      ),
      child: Row(
        children: [
          const Icon(Icons.cloud_off, color: Colors.white24, size: 16),
          const SizedBox(width: OpenTokenTheme.space2),
          Expanded(
            child: Text(
              "Offline First. No data leaves this machine.",
              style: GoogleFonts.inter(color: Colors.white24, fontSize: 11),
            ),
          ),
          TextButton(
            onPressed: () {},
            child: Text(
              "View Source on GitHub",
              style: GoogleFonts.inter(
                color: OpenTokenTheme.primary,
                fontSize: 11,
                fontWeight: OpenTokenTheme.fontSemibold,
              ),
            ),
          ),
        ],
      ),
    );
  }

  void _showFactoryResetDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: OpenTokenTheme.surfaceCard,
        title: const Row(
          children: [
            Icon(Icons.warning_amber_rounded, color: OpenTokenTheme.error),
            SizedBox(width: 12),
            Text("Factory Reset"),
          ],
        ),
        content: const Text(
          "This will permanently erase all secrets, keys, and configuration data from the device. This action cannot be undone.",
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text("Cancel"),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Trigger factory reset
            },
            style: OpenTokenTheme.dangerButton,
            child: const Text("Erase Device"),
          ),
        ],
      ),
    );
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPER WIDGETS
// ═══════════════════════════════════════════════════════════════════════════

class _SectionHeader extends StatelessWidget {
  final String title;
  const _SectionHeader({required this.title});

  @override
  Widget build(BuildContext context) {
    return Text(
      title,
      style: GoogleFonts.inter(
        color: Colors.white,
        fontSize: OpenTokenTheme.textBase,
        fontWeight: OpenTokenTheme.fontSemibold,
      ),
    );
  }
}

class _NavigationTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;
  final Color? iconColor;
  final VoidCallback onTap;

  const _NavigationTile({
    required this.title,
    required this.subtitle,
    required this.icon,
    this.iconColor,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: OpenTokenTheme.space3),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.02),
        borderRadius: OpenTokenTheme.borderRadiusLg,
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: OpenTokenTheme.borderRadiusLg,
          child: Padding(
            padding: const EdgeInsets.all(OpenTokenTheme.space4),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color:
                        (iconColor ?? OpenTokenTheme.primary).withOpacity(0.1),
                    borderRadius: OpenTokenTheme.borderRadiusMd,
                  ),
                  child: Icon(
                    icon,
                    color: iconColor ?? OpenTokenTheme.primary,
                    size: 20,
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
                          fontWeight: OpenTokenTheme.fontMedium,
                          fontSize: OpenTokenTheme.textBase,
                        ),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        subtitle,
                        style: GoogleFonts.inter(
                          color: Colors.white38,
                          fontSize: OpenTokenTheme.textSm,
                        ),
                      ),
                    ],
                  ),
                ),
                const Icon(
                  Icons.chevron_right,
                  color: Colors.white24,
                  size: 20,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _SettingsTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final Widget child;

  const _SettingsTile({
    required this.title,
    required this.subtitle,
    required this.child,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: OpenTokenTheme.space3),
      padding: const EdgeInsets.all(OpenTokenTheme.space4),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.02),
        borderRadius: OpenTokenTheme.borderRadiusLg,
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: GoogleFonts.inter(
                    color: Colors.white,
                    fontWeight: OpenTokenTheme.fontMedium,
                    fontSize: OpenTokenTheme.textBase,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  subtitle,
                  style: GoogleFonts.inter(
                    color: Colors.white38,
                    fontSize: OpenTokenTheme.textSm,
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(width: OpenTokenTheme.space4),
          child,
        ],
      ),
    );
  }
}

class _ToggleTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final bool value;
  final ValueChanged<bool> onChanged;

  const _ToggleTile({
    required this.title,
    required this.subtitle,
    required this.value,
    required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: OpenTokenTheme.space3),
      padding: const EdgeInsets.all(OpenTokenTheme.space4),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.02),
        borderRadius: OpenTokenTheme.borderRadiusLg,
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: GoogleFonts.inter(
                    color: Colors.white,
                    fontWeight: OpenTokenTheme.fontMedium,
                    fontSize: OpenTokenTheme.textBase,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  subtitle,
                  style: GoogleFonts.inter(
                    color: Colors.white38,
                    fontSize: OpenTokenTheme.textSm,
                  ),
                ),
              ],
            ),
          ),
          Switch(
            value: value,
            onChanged: onChanged,
            activeThumbColor: OpenTokenTheme.primary,
            activeTrackColor: OpenTokenTheme.primary.withOpacity(0.3),
            inactiveThumbColor: Colors.white38,
            inactiveTrackColor: Colors.white.withOpacity(0.1),
          ),
        ],
      ),
    );
  }
}
