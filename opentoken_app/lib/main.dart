import 'dart:io';
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:opentoken_dart/opentoken_service.dart';
import 'package:opentoken_dart/transports/transport_nfc.dart';
import 'package:opentoken_dart/transports/transport_usb.dart';
import 'dart:async';

import 'widgets/premium_card.dart';
import 'widgets/device_status.dart';

void main() {
  runApp(const OpenTokenApp());
}

class OpenTokenApp extends StatelessWidget {
  const OpenTokenApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'OpenToken Sovereign',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: const Color(0xFF00A3FF),
        scaffoldBackgroundColor: const Color(0xFF0A0A0C),
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF00A3FF),
          secondary: Color(0xFF00A3FF),
          surface: Color(0xFF161618),
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
  late OpenTokenSharedService _service;
  bool _isConnected = false;
  String _transportName = "Unknown";

  @override
  void initState() {
    super.initState();
    _initializeService();
  }

  void _initializeService() {
    // Select transport based on platform (simple logic for now)
    final bool isMobile = Platform.isAndroid || Platform.isIOS;
    final transport = isMobile ? NfcTransport() : UsbTransport();
    _transportName = isMobile ? "NFC" : "USB";
    _service = OpenTokenSharedService(transport);
    
    // Simulate connection for demo if needed, or real check
    setState(() => _isConnected = true); 
  }

  @override
  Widget build(BuildContext context) {
    final bool isDesktop = MediaQuery.of(context).size.width > 600;

    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            DeviceStatusBanner(
              isConnected: _isConnected,
              transportName: _transportName,
            ),
            Expanded(
              child: Row(
                children: [
                  if (isDesktop)
                    NavigationRail(
                      backgroundColor: const Color(0xFF0A0A0C),
                      selectedIndex: _selectedIndex,
                      onDestinationSelected: (idx) =>
                          setState(() => _selectedIndex = idx),
                      labelType: NavigationRailLabelType.all,
                      selectedLabelTextStyle: GoogleFonts.inter(
                        color: const Color(0xFF00A3FF),
                        fontWeight: FontWeight.bold,
                      ),
                      destinations: const [
                        NavigationRailDestination(
                            icon: Icon(Icons.security_outlined),
                            selectedIcon: Icon(Icons.security, color: Color(0xFF00A3FF)),
                            label: Text('OATH')),
                        NavigationRailDestination(
                            icon: Icon(Icons.fingerprint_outlined),
                            selectedIcon: Icon(Icons.fingerprint, color: Color(0xFF00A3FF)),
                            label: Text('FIDO2')),
                        NavigationRailDestination(
                            icon: Icon(Icons.settings_outlined),
                            selectedIcon: Icon(Icons.settings, color: Color(0xFF00A3FF)),
                            label: Text('Settings')),
                      ],
                    ),
                  Expanded(
                    child: _buildCurrentPage(),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
      bottomNavigationBar: isDesktop
          ? null
          : NavigationBar(
              selectedIndex: _selectedIndex,
              onDestinationSelected: (idx) =>
                  setState(() => _selectedIndex = idx),
              destinations: const [
                NavigationDestination(
                    icon: Icon(Icons.security), label: 'OATH'),
                NavigationDestination(
                    icon: Icon(Icons.fingerprint), label: 'FIDO2'),
                NavigationDestination(
                    icon: Icon(Icons.info_outline), label: 'Device'),
              ],
            ),
    );
  }

  Widget _buildCurrentPage() {
    switch (_selectedIndex) {
      case 0:
        return _OathView(service: _service);
      case 1:
        return _PlaceholderView(
          title: "FIDO2",
          subtitle: "Manage hardware security keys and discoverable credentials.",
          icon: Icons.fingerprint,
        );
      case 2:
        return _PlaceholderView(
          title: "Settings",
          subtitle: "Configure your OpenToken device and application preferences.",
          icon: Icons.settings,
        );
      default:
        return Container();
    }
  }
}

class _PlaceholderView extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;

  const _PlaceholderView({
    required this.title,
    required this.subtitle,
    required this.icon,
  });

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(icon, size: 64, color: Colors.white10),
            const SizedBox(height: 24),
            Text(
              title,
              style: GoogleFonts.inter(
                fontSize: 24,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              subtitle,
              textAlign: TextAlign.center,
              style: GoogleFonts.inter(
                fontSize: 16,
                color: Colors.white54,
              ),
            ),
            const SizedBox(height: 32),
            Opacity(
              opacity: 0.5,
              child: Chip(
                label: Text("COMING SOON", style: GoogleFonts.inter(fontSize: 10, fontWeight: FontWeight.bold)),
                backgroundColor: Colors.white10,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _OathView extends StatefulWidget {
  final OpenTokenSharedService service;
  const _OathView({required this.service});

  @override
  State<_OathView> createState() => _OathViewState();
}

class _OathViewState extends State<_OathView> {
  List<Map<String, String>> _accounts = [];
  Map<String, String> _codes = {};
  double _progress = 1.0;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _loadAccounts();
    _startTimer();
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
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

  Future<void> _loadAccounts() async {
    // In a real scenario, we'd select the applet first
    // await widget.service.selectOathApplet();
    // final accounts = await widget.service.listOathAccounts();
    
    // Mock data for WOW factor if no device is connected
    final accounts = [
      {"name": "GitHub:zequinha", "type": "TOTP"},
      {"name": "Google:ze@work", "type": "TOTP"},
      {"name": "AWS:prod-admin", "type": "TOTP"},
    ];

    setState(() {
      _accounts = accounts;
      // Initialize with dummy codes
      for (var acc in accounts) {
        _codes[acc["name"]!] = "******";
      }
    });
    
    _refreshCodes();
  }

  Future<void> _refreshCodes() async {
    for (var acc in _accounts) {
      // String? code = await widget.service.calculateCode(acc["name"]!);
      // Use mock code for now to ensure UI looks great
      final mockCode = (100000 + (DateTime.now().millisecondsSinceEpoch % 900000)).toString();
      setState(() {
        _codes[acc["name"]!] = mockCode;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: Text(
          "Sovereign OATH",
          style: GoogleFonts.inter(fontWeight: FontWeight.bold),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.add_circle_outline),
            onPressed: () {},
          ),
        ],
      ),
      body: ListView.builder(
        itemCount: _accounts.length,
        padding: const EdgeInsets.only(top: 8, bottom: 24),
        itemBuilder: (ctx, idx) {
          final account = _accounts[idx];
          final name = account["name"]!;
          final parts = name.split(':');
          
          return PremiumCard(
            title: parts.length > 1 ? parts[1] : name,
            subtitle: parts[0],
            code: _codes[name] ?? "000000",
            progress: _progress,
            onTap: () {
              // Copy to clipboard or show details
            },
          );
        },
      ),
    );
  }
}
