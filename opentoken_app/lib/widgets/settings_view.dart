import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class SettingsView extends StatelessWidget {
  const SettingsView({super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(40.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            "Settings",
            style: GoogleFonts.inter(
                fontSize: 32, fontWeight: FontWeight.bold, color: Colors.white),
          ),
          const SizedBox(height: 8),
          Text(
            "Configure local application preferences and device security.",
            style: GoogleFonts.inter(color: Colors.white54, fontSize: 14),
          ),
          const SizedBox(height: 48),
          _buildDeviceHeader(),
          const SizedBox(height: 40),
          _SectionHeader(icon: Icons.security_outlined, title: "Security"),
          const SizedBox(height: 16),
          _buildSettingsTile(
            title: "Device PIN",
            subtitle: "Required to access keys. Last changed 30 days ago.",
            action: ElevatedButton(
              onPressed: () {},
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF007BFF),
                foregroundColor: Colors.white,
                shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(8)),
              ),
              child: const Text("Update PIN"),
            ),
          ),
          _buildSettingsTile(
            title: "Auto-lock Timeout",
            subtitle: "Automatically lock the application after inactivity.",
            action: _buildDropdown("5 Minutes"),
          ),
          const SizedBox(height: 40),
          _SectionHeader(icon: Icons.palette_outlined, title: "Interface"),
          const SizedBox(height: 16),
          _buildSettingsTile(
            title: "App Theme",
            subtitle: "Choose your preferred visual appearance.",
            action: _buildSegmentedControl([
              {"icon": Icons.light_mode, "label": "Light"},
              {"icon": Icons.dark_mode, "label": "Dark"},
              {"icon": Icons.settings_brightness, "label": "System"},
            ], "Dark"),
          ),
          _buildSettingsTile(
            title: "Language",
            subtitle: "Select the application language.",
            action: _buildDropdown("English (US)"),
          ),
          const Spacer(),
          const Divider(color: Colors.white10),
          const SizedBox(height: 24),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text("OpenToken Authenticator v1.0.0",
                      style: GoogleFonts.inter(
                          color: Colors.white24, fontSize: 12)),
                  const SizedBox(height: 4),
                  Text("View Source on GitHub",
                      style: GoogleFonts.inter(
                          color: const Color(0xFF007BFF),
                          fontSize: 11,
                          fontWeight: FontWeight.bold)),
                ],
              ),
              Container(
                padding:
                    const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.03),
                    borderRadius: BorderRadius.circular(20)),
                child: Row(
                  children: [
                    const Icon(Icons.cloud_off,
                        color: Colors.white24, size: 14),
                    const SizedBox(width: 8),
                    Text("Offline First. No data leaves this machine.",
                        style: GoogleFonts.inter(
                            color: Colors.white24, fontSize: 10)),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceHeader() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [
            Colors.white.withOpacity(0.05),
            Colors.white.withOpacity(0.01)
          ],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.08)),
      ),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
                color: const Color(0xFF007BFF).withOpacity(0.1),
                shape: BoxShape.circle),
            child: const Icon(Icons.usb, color: Color(0xFF007BFF)),
          ),
          const SizedBox(width: 20),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text("RP2350 Secure Token",
                    style: GoogleFonts.inter(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        fontSize: 16)),
                const SizedBox(height: 4),
                Row(
                  children: [
                    const Icon(Icons.check_circle,
                        color: Colors.white24, size: 14),
                    const SizedBox(width: 6),
                    Text("Firmware v2.4.1 (Stable)",
                        style: GoogleFonts.inter(
                            color: Colors.white38, fontSize: 12)),
                  ],
                ),
              ],
            ),
          ),
          Column(
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              Text("SERIAL NUMBER",
                  style: GoogleFonts.inter(
                      color: Colors.white24,
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1)),
              const SizedBox(height: 4),
              Text("XXXX-XXXX-A7B2-99F1",
                  style: GoogleFonts.jetBrainsMono(
                      color: Colors.white, fontSize: 12)),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSettingsTile(
      {required String title,
      required String subtitle,
      required Widget action}) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 20),
      decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.02),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: Colors.white.withOpacity(0.05))),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(title,
                    style: GoogleFonts.inter(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        fontSize: 15)),
                const SizedBox(height: 4),
                Text(subtitle,
                    style:
                        GoogleFonts.inter(color: Colors.white38, fontSize: 13)),
              ],
            ),
          ),
          const SizedBox(width: 24),
          action,
        ],
      ),
    );
  }

  Widget _buildDropdown(String text) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: Colors.white.withOpacity(0.1))),
      child: Row(
        children: [
          Text(text,
              style: GoogleFonts.inter(color: Colors.white70, fontSize: 13)),
          const SizedBox(width: 8),
          const Icon(Icons.keyboard_arrow_down,
              color: Colors.white38, size: 18),
        ],
      ),
    );
  }

  Widget _buildSegmentedControl(
      List<Map<String, dynamic>> items, String selected) {
    return Container(
      padding: const EdgeInsets.all(4),
      decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(8)),
      child: Row(
        children: items.map((item) {
          final isSelected = item["label"] == selected;
          return Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
                color: isSelected
                    ? Colors.white.withOpacity(0.1)
                    : Colors.transparent,
                borderRadius: BorderRadius.circular(6)),
            child: Row(
              children: [
                Icon(item["icon"],
                    size: 14,
                    color: isSelected ? Colors.white : Colors.white38),
                const SizedBox(width: 6),
                Text(item["label"],
                    style: GoogleFonts.inter(
                        color: isSelected ? Colors.white : Colors.white38,
                        fontSize: 11,
                        fontWeight:
                            isSelected ? FontWeight.bold : FontWeight.normal)),
              ],
            ),
          );
        }).toList(),
      ),
    );
  }
}

class _SectionHeader extends StatelessWidget {
  final IconData icon;
  final String title;
  const _SectionHeader({required this.icon, required this.title});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(icon, color: const Color(0xFF007BFF), size: 18),
        const SizedBox(width: 12),
        Text(title,
            style: GoogleFonts.inter(
                color: Colors.white,
                fontSize: 16,
                fontWeight: FontWeight.bold)),
      ],
    );
  }
}
