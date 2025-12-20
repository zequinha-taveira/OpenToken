import 'dart:io';
import 'package:flutter/material.dart';
import 'package:opentoken_dart/opentoken_service.dart';
import 'package:opentoken_dart/transports/transport_nfc.dart';
import 'package:opentoken_dart/transports/transport_usb.dart';

void main() {
  runApp(const OpenTokenApp());
}

class OpenTokenApp extends StatelessWidget {
  const OpenTokenApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'OpenToken Sovereign',
      theme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: const Color(0xFF007ACC),
        scaffoldBackgroundColor: const Color(0xFF121212),
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

  @override
  void initState() {
    super.initState();
    // Select transport based on platform
    final transport = (Platform.isAndroid || Platform.isIOS)
        ? NfcTransport()
        : UsbTransport();
    _service = OpenTokenSharedService(transport);
  }

  @override
  Widget build(BuildContext context) {
    final bool isDesktop = MediaQuery.of(context).size.width > 600;

    return Scaffold(
      body: Row(
        children: [
          if (isDesktop)
            NavigationRail(
              selectedIndex: _selectedIndex,
              onDestinationSelected: (idx) =>
                  setState(() => _selectedIndex = idx),
              labelType: NavigationRailLabelType.all,
              destinations: const [
                NavigationRailDestination(
                    icon: Icon(Icons.security), label: Text('OATH')),
                NavigationRailDestination(
                    icon: Icon(Icons.fingerprint), label: Text('FIDO2')),
                NavigationRailDestination(
                    icon: Icon(Icons.info_outline), label: Text('Device')),
              ],
            ),
          Expanded(
            child: _buildCurrentPage(),
          ),
        ],
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
        return const Center(child: Text("FIDO2 Management (Commning Soon)"));
      case 2:
        return const Center(child: Text("Device Info: OpenToken RP2350"));
      default:
        return Container();
    }
  }
}

class _OathView extends StatelessWidget {
  final OpenTokenSharedService service;
  const _OathView({required this.service});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("OATH Credentials")),
      body: ListView.builder(
        itemCount: 2,
        itemBuilder: (ctx, idx) => Card(
          margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          child: ListTile(
            title: Text(idx == 0 ? "GitHub:user" : "Google:user"),
            trailing: const Text("123 456",
                style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
          ),
        ),
      ),
    );
  }
}
