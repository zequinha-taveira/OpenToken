import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'dart:async';

import 'theme.dart';
import 'widgets/credentials_view.dart';
import 'widgets/add_credential_view.dart';
import 'widgets/crypto_management_view.dart';
import 'widgets/device_status_view.dart';
import 'widgets/settings_view.dart';

void main() {
  runApp(const OpenTokenApp());
}

class OpenTokenApp extends StatelessWidget {
  const OpenTokenApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'OpenToken NATIVO',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: OpenTokenTheme.electricPurple,
        scaffoldBackgroundColor: OpenTokenTheme.deepBackground,
        colorScheme: const ColorScheme.dark(
          primary: OpenTokenTheme.electricPurple,
          secondary: OpenTokenTheme.cyanPulse,
          surface: OpenTokenTheme.surfaceCard,
        ),
        textTheme: GoogleFonts.interTextTheme(ThemeData.dark().textTheme),
        useMaterial3: true,
      ),
      home: const MainNavigation(),
    );
  }
}

class MainNavigation extends StatefulWidget {
  const MainNavigation({super.key});

  @override
  State<MainNavigation> createState() => _MainNavigationState();
}

class _MainNavigationState extends State<MainNavigation> {
  int _selectedIndex = 0;
  bool _isAddingCredential = false;
  
  // Mock data for OATH
  List<Map<String, String>> _accounts = [
    {"name": "GitHub:zequinha", "type": "TOTP"},
    {"name": "Google:ze@work", "type": "TOTP"},
    {"name": "AWS:prod-admin", "type": "TOTP"},
    {"name": "ProtonMail:privacy", "type": "TOTP"},
  ];
  Map<String, String> _codes = {};
  double _progress = 1.0;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _startTimer();
    _refreshCodes();
  }

  void _startTimer() {
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      final now = DateTime.now().second;
      setState(() {
        _progress = 1.0 - ((now % 30) / 30.0);
      });
      if (now % 30 == 0) {
        _refreshCodes();
      }
    });
  }

  void _refreshCodes() {
    for (var acc in _accounts) {
      final mockCode = (100000 + (DateTime.now().millisecondsSinceEpoch % 900000)).toString();
      setState(() {
        _codes[acc["name"]!] = mockCode;
      });
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_isAddingCredential) {
      return AddCredentialView(
        onCancel: () => setState(() => _isAddingCredential = false),
        onSave: (name, secret, algo, digits) {
          setState(() {
            _accounts.add({"name": name, "type": "TOTP"});
            _isAddingCredential = false;
          });
          _refreshCodes();
        },
      );
    }

    final bool isDesktop = MediaQuery.of(context).size.width > 900;

    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            _buildTopBar(),
            Expanded(
              child: Row(
                children: [
                  if (isDesktop) _buildSidebar(),
                  Expanded(
                    child: Container(
                      decoration: BoxDecoration(
                        color: Colors.white.withOpacity(0.01),
                        border: Border(left: BorderSide(color: Colors.white.withOpacity(0.05))),
                      ),
                      child: _buildCurrentPage(),
                    ),
                  ),
                ],
              ),
            ),
            _buildFooter(),
          ],
        ),
      ),
      bottomNavigationBar: isDesktop
          ? null
          : NavigationBar(
              selectedIndex: _selectedIndex,
              onDestinationSelected: (idx) => setState(() => _selectedIndex = idx),
              backgroundColor: const Color(0xFF0A0A0C),
              indicatorColor: OpenTokenTheme.electricPurple.withOpacity(0.2),
              destinations: const [
                NavigationDestination(icon: Icon(Icons.security), label: 'Credentials'),
                NavigationDestination(icon: Icon(Icons.fingerprint), label: 'Crypto'),
                NavigationDestination(icon: Icon(Icons.info_outline), label: 'Device'),
                NavigationDestination(icon: Icon(Icons.settings), label: 'Settings'),
              ],
            ),
    );
  }

  Widget _buildTopBar() {
    return Container(
      height: 64,
      padding: const EdgeInsets.symmetric(horizontal: 24),
      decoration: BoxDecoration(
        color: const Color(0xFF050505),
        border: Border(bottom: BorderSide(color: Colors.white.withOpacity(0.05))),
      ),
      child: Row(
        children: [
          const Icon(Icons.shield, color: OpenTokenTheme.electricPurple, size: 28),
          const SizedBox(width: 12),
          Text(
            "OpenToken Authenticator",
            style: GoogleFonts.inter(fontWeight: FontWeight.bold, fontSize: 18, color: Colors.white),
          ),
          const Spacer(),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: const Color(0xFF00F0FF).withOpacity(0.1),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: const Color(0xFF00F0FF).withOpacity(0.2)),
            ),
            child: Row(
              children: [
                Container(
                  width: 8,
                  height: 8,
                  decoration: const BoxDecoration(color: Color(0xFF00F0FF), shape: BoxShape.circle),
                ),
                const SizedBox(width: 8),
                Text(
                  "Device: Connected (RP2350)",
                  style: GoogleFonts.inter(color: const Color(0xFF00F0FF), fontSize: 11, fontWeight: FontWeight.bold),
                ),
              ],
            ),
          ),
          const SizedBox(width: 16),
          const Icon(Icons.lock_outline, color: Colors.white38, size: 20),
        ],
      ),
    );
  }

  Widget _buildSidebar() {
    return Container(
      width: 240,
      color: const Color(0xFF050505),
      child: Column(
        children: [
          const SizedBox(height: 24),
          _SidebarItem(
            icon: Icons.security_outlined,
            label: "Credentials",
            isSelected: _selectedIndex == 0,
            onTap: () => setState(() => _selectedIndex = 0),
          ),
          _SidebarItem(
            icon: Icons.fingerprint_outlined,
            label: "Crypto",
            isSelected: _selectedIndex == 1,
            onTap: () => setState(() => _selectedIndex = 1),
          ),
          _SidebarItem(
            icon: Icons.info_outline,
            label: "Device Status",
            isSelected: _selectedIndex == 2,
            onTap: () => setState(() => _selectedIndex = 2),
          ),
          _SidebarItem(
            icon: Icons.settings_outlined,
            label: "Settings",
            isSelected: _selectedIndex == 3,
            onTap: () => setState(() => _selectedIndex = 3),
          ),
        ],
      ),
    );
  }

  Widget _buildCurrentPage() {
    switch (_selectedIndex) {
      case 0:
        return CredentialsView(
          accounts: _accounts,
          codes: _codes,
          progress: _progress,
          onAdd: () => setState(() => _isAddingCredential = true),
        );
      case 1:
        return const CryptoManagementView();
      case 2:
        return const DeviceStatusView();
      case 3:
        return const SettingsView();
      default:
        return Container();
    }
  }

  Widget _buildFooter() {
    return Container(
      height: 32,
      padding: const EdgeInsets.symmetric(horizontal: 16),
      decoration: BoxDecoration(
        color: const Color(0xFF020202),
        border: Border(top: BorderSide(color: Colors.white.withOpacity(0.05))),
      ),
      child: Row(
        children: [
          _FooterItem(icon: Icons.usb, label: "USB: OK", color: const Color(0xFF00F0FF)),
          const SizedBox(width: 24),
          _FooterItem(icon: Icons.code, label: "FW: V0.3.1-BETA"),
          const SizedBox(width: 24),
          _FooterItem(icon: Icons.terminal, label: "PROTOCOL: V1.0"),
          const Spacer(),
          _FooterItem(icon: Icons.battery_charging_full, label: "BAT: 98%"),
          const SizedBox(width: 24),
          _FooterItem(label: "SERIAL: RP-2350-XJ92"),
        ],
      ),
    );
  }
}

class _SidebarItem extends StatelessWidget {
  final IconData icon;
  final String label;
  final bool isSelected;
  final VoidCallback onTap;

  const _SidebarItem({
    required this.icon,
    required this.label,
    required this.isSelected,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          child: Row(
            children: [
              Icon(
                icon,
                color: isSelected ? OpenTokenTheme.electricPurple : Colors.white38,
                size: 20,
              ),
              const SizedBox(width: 16),
              Text(
                label,
                style: GoogleFonts.inter(
                  color: isSelected ? Colors.white : Colors.white38,
                  fontSize: 14,
                  fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
                ),
              ),
              if (isSelected) ...[
                const Spacer(),
                Container(
                  width: 4,
                  height: 4,
                  decoration: const BoxDecoration(color: OpenTokenTheme.electricPurple, shape: BoxShape.circle),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}

class _FooterItem extends StatelessWidget {
  final IconData? icon;
  final String label;
  final Color? color;

  const _FooterItem({this.icon, required this.label, this.color});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        if (icon != null) ...[
          Icon(icon, size: 12, color: color ?? Colors.white24),
          const SizedBox(width: 6),
        ],
        Text(
          label,
          style: GoogleFonts.jetBrainsMono(
            color: color?.withOpacity(0.5) ?? Colors.white24,
            fontSize: 9,
            fontWeight: FontWeight.bold,
          ),
        ),
      ],
    );
  }
}
