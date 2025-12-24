import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:opentoken_dart/opentoken_service.dart';
import 'package:opentoken_dart/transports/transport_usb.dart';
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
      title: 'OpenToken Authenticator',
      debugShowCheckedModeBanner: false,
      theme: OpenTokenTheme.darkTheme,
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

  // Global USB configuration
  static const int usbVid = 0xCAFE;
  static const int usbPid = 0x4004;

  // Device state
  late UsbTransport _transport;
  late OpenTokenSharedService _service;
  bool _isConnected = false;
  String _transportName = "USB";
  String _firmwareVersion = "Unknown";
  String _serialNumber = "Not Connected";
  int _slotsUsed = 0;
  int _slotsMax = 200;

  // Credential data (from device)
  List<Map<String, String>> _accounts = [];
  Map<String, String> _codes = {};
  double _progress = 1.0;
  Timer? _timer;
  Timer? _devicePollTimer;
  bool _isPinPromptVisible = false;
  bool _isRefreshing = false;
  Map<dynamic, dynamic>? _fidoInfo;

  @override
  void initState() {
    super.initState();
    _initializeDevice();
    _startTimer();
  }

  /// Initialize and connect to OpenToken device
  Future<void> _initializeDevice() async {
    // Desktop uses USB HID
    _transportName = "USB";
    _transport = UsbTransport(vendorId: usbVid, productId: usbPid);
    _service = OpenTokenSharedService(_transport);

    // Try to connect to device
    await _connectToDevice();

    // Start polling for device connection changes
    _devicePollTimer = Timer.periodic(const Duration(seconds: 2), (_) {
      _checkDeviceConnection();
    });
  }

  /// Connect to USB device
  Future<void> _connectToDevice() async {
    try {
      final connected = await _transport.connect();
      if (connected) {
        // Select OATH applet
        final selected = await _service.selectOathApplet();
        if (selected) {
          setState(() => _isConnected = true);
          await _refreshFromDevice();
        }
      } else {
        setState(() => _isConnected = false);
      }
    } catch (e) {
      debugPrint('Device connection error: $e');
      setState(() => _isConnected = false);
    }
  }

  /// Check if device is still connected
  Future<void> _checkDeviceConnection() async {
    if (!_isConnected) {
      // Try to reconnect
      await _connectToDevice();
    }
  }

  /// Refresh all data from device
  Future<void> _refreshFromDevice() async {
    await _fetchFirmwareInfo();
    await _fetchDeviceStatus();
    await _fetchFidoInfo();
    await _loadAccountsFromDevice();
  }

  Future<void> _fetchFidoInfo() async {
    if (!_isConnected) return;
    try {
      final info = await _service.getFidoInfo();
      if (mounted) {
        setState(() => _fidoInfo = info);
      }
    } catch (e) {
      debugPrint('Error fetching FIDO info: $e');
    }
  }

  /// Load accounts from device and calculate codes
  Future<void> _loadAccountsFromDevice() async {
    if (!_isConnected) return;

    try {
      final accounts = await _service.listOathAccounts();
      setState(() => _accounts = accounts);

      // Calculate codes for all accounts
      await _refreshCodes();
    } catch (e) {
      debugPrint('Error loading accounts: $e');
    }
  }

  Future<void> _fetchFirmwareInfo() async {
    if (_isConnected) {
      final version = await _service.getFirmwareVersion();
      setState(() {
        _firmwareVersion = "v$version";
      });
    }
  }

  /// Fetch device status (slot counts)
  Future<void> _fetchDeviceStatus() async {
    if (!_isConnected) return;

    try {
      final status = await _service.getDeviceStatus();
      if (status != null) {
        setState(() {
          _slotsUsed = status.oathCount;
          _slotsMax = status.oathMax > 0 ? status.oathMax : 200;
        });
      }
    } catch (e) {
      debugPrint('Error fetching device status: $e');
    }
  }

  Future<void> _triggerReboot() async {
    if (_isConnected) {
      final success = await _service.enterBootloaderMode();
      if (success) {
        setState(() => _isConnected = false);
      }
    }
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

  Future<void> _refreshCodes() async {
    if (!_isConnected) {
      // Use mock codes when not connected
      for (var acc in _accounts) {
        final mockCode =
            (100000 + (DateTime.now().millisecondsSinceEpoch % 900000))
                .toString();
        _codes[acc["name"]!] = mockCode;
      }
      setState(() {});
      return;
    }

    // Fetch real codes from device
    for (var acc in _accounts) {
      final name = acc["name"]!;
      try {
        final code = await _service.calculateCode(name);
        if (code != null) {
          setState(() {
            _codes[name] = code.code;
          });
        }
      } on PinRequiredException {
        if (!_isPinPromptVisible) {
          _showPinDialog();
        }
        break; // Stop trying to fetch other codes until unlocked
      } catch (e) {
        debugPrint('Error calculating code for $name: $e');
      }
    }
  }

  Future<void> _showPinDialog() async {
    if (_isPinPromptVisible) return;
    setState(() => _isPinPromptVisible = true);

    final pinController = TextEditingController();
    bool isLoading = false;
    String? errorMessage;

    await showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => StatefulBuilder(
        builder: (context, setDialogState) => AlertDialog(
          backgroundColor: OpenTokenTheme.deepBackground,
          shape: RoundedRectangleBorder(
              borderRadius: OpenTokenTheme.borderRadiusLg,
              side: BorderSide(color: Colors.white.withOpacity(0.1))),
          title: Row(
            children: [
              const Icon(Icons.lock_outline, color: OpenTokenTheme.primary),
              const SizedBox(width: 12),
              Text(
                "Device Locked",
                style: GoogleFonts.inter(color: Colors.white),
              ),
            ],
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                "Your OpenToken requires a PIN to access these credentials.",
                style: GoogleFonts.inter(color: Colors.white70, fontSize: 13),
              ),
              const SizedBox(height: 16),
              TextField(
                controller: pinController,
                obscureText: true,
                style: const TextStyle(color: Colors.white),
                decoration: InputDecoration(
                  hintText: "Enter Device PIN",
                  errorText: errorMessage,
                ),
                autofocus: true,
                onSubmitted: (_) async {
                  // Trigger validation
                },
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.pop(context);
              },
              child: const Text("Cancel"),
            ),
            ElevatedButton(
              onPressed: isLoading
                  ? null
                  : () async {
                      setDialogState(() {
                        isLoading = true;
                        errorMessage = null;
                      });

                      try {
                        final success =
                            await _service.validateOathPin(pinController.text);
                        if (success) {
                          Navigator.pop(context);
                          _refreshCodes();
                        } else {
                          setDialogState(() {
                            errorMessage = "Incorrect PIN";
                            isLoading = false;
                          });
                        }
                      } catch (e) {
                        setDialogState(() {
                          errorMessage = "Error validating PIN";
                          isLoading = false;
                        });
                      }
                    },
              child: isLoading
                  ? const SizedBox(
                      width: 16,
                      height: 16,
                      child: CircularProgressIndicator(
                          strokeWidth: 2, color: Colors.white))
                  : const Text("Unlock"),
            ),
          ],
        ),
      ),
    );

    setState(() => _isPinPromptVisible = false);
  }

  Future<void> _manualRefresh() async {
    if (_isRefreshing) return;
    setState(() => _isRefreshing = true);
    try {
      await _refreshFromDevice();
    } finally {
      if (mounted) {
        setState(() => _isRefreshing = false);
      }
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _devicePollTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_isAddingCredential) {
      return AddCredentialView(
        service: _service,
        onCancel: () => setState(() => _isAddingCredential = false),
        onSuccess: () {
          setState(() => _isAddingCredential = false);
          _refreshFromDevice();
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
                        border: Border(
                            left: BorderSide(
                                color: Colors.white.withOpacity(0.05))),
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
              onDestinationSelected: (idx) =>
                  setState(() => _selectedIndex = idx),
              backgroundColor: const Color(0xFF0A0A0C),
              indicatorColor: OpenTokenTheme.primary.withOpacity(0.2),
              destinations: const [
                NavigationDestination(
                    icon: Icon(Icons.security), label: 'Credenciais'),
                NavigationDestination(
                    icon: Icon(Icons.fingerprint), label: 'Cripto'),
                NavigationDestination(
                    icon: Icon(Icons.info_outline), label: 'Dispositivo'),
                NavigationDestination(
                    icon: Icon(Icons.settings), label: 'Configurações'),
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
          isConnected: _isConnected,
          serial: _serialNumber,
          firmwareVersion: _firmwareVersion.replaceAll('v', ''),
          slotsUsed: _slotsUsed,
          slotsTotal: _slotsMax,
          isRefreshing: _isRefreshing,
          onRefresh: _manualRefresh,
        );
      case 1:
        return CryptoManagementView(fidoInfo: _fidoInfo);
      case 2:
        return DeviceStatusView(
          firmwareVersion: _firmwareVersion,
          isConnected: _isConnected,
          onReboot: _triggerReboot,
        );
      case 3:
        return const SettingsView();
      default:
        return Container();
    }
  }

  Widget _buildTopBar() {
    return Container(
      height: 64,
      padding: const EdgeInsets.symmetric(horizontal: 24),
      decoration: BoxDecoration(
        color: const Color(0xFF050505),
        border:
            Border(bottom: BorderSide(color: Colors.white.withOpacity(0.05))),
      ),
      child: Row(
        children: [
          Icon(Icons.shield, color: OpenTokenTheme.primary, size: 28),
          const SizedBox(width: 12),
          Text(
            "OpenToken Authenticator",
            style: GoogleFonts.inter(
                fontWeight: FontWeight.bold, fontSize: 18, color: Colors.white),
          ),
          const Spacer(),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: _isConnected
                  ? OpenTokenTheme.secondary.withOpacity(0.1)
                  : OpenTokenTheme.error.withOpacity(0.1),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(
                  color: _isConnected
                      ? OpenTokenTheme.secondary.withOpacity(0.2)
                      : OpenTokenTheme.error.withOpacity(0.2)),
            ),
            child: Row(
              children: [
                Container(
                  width: 8,
                  height: 8,
                  decoration: BoxDecoration(
                      color: _isConnected
                          ? OpenTokenTheme.secondary
                          : OpenTokenTheme.error,
                      shape: BoxShape.circle),
                ),
                const SizedBox(width: 8),
                Text(
                  _isConnected
                      ? "Device: Connected (RP2350)"
                      : "Device: Disconnected",
                  style: GoogleFonts.inter(
                      color: _isConnected
                          ? OpenTokenTheme.secondary
                          : OpenTokenTheme.error,
                      fontSize: 11,
                      fontWeight: FontWeight.bold),
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
          _SidebarItem(
            icon: Icons.security_outlined,
            label: "Credenciais",
            isSelected: _selectedIndex == 0,
            onTap: () => setState(() => _selectedIndex = 0),
          ),
          _SidebarItem(
            icon: Icons.fingerprint_outlined,
            label: "Criptografia",
            isSelected: _selectedIndex == 1,
            onTap: () => setState(() => _selectedIndex = 1),
          ),
          _SidebarItem(
            icon: Icons.info_outline,
            label: "Status do Dispositivo",
            isSelected: _selectedIndex == 2,
            onTap: () => setState(() => _selectedIndex = 2),
          ),
          _SidebarItem(
            icon: Icons.settings_outlined,
            label: "Configurações",
            isSelected: _selectedIndex == 3,
            onTap: () => setState(() => _selectedIndex = 3),
          ),
        ],
      ),
    );
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
          _FooterItem(
              icon: Icons.usb,
              label: "USB: ${_isConnected ? 'OK' : 'ERR'}",
              color: _isConnected
                  ? OpenTokenTheme.secondary
                  : OpenTokenTheme.error),
          const SizedBox(width: 24),
          _FooterItem(icon: Icons.code, label: "FW: $_firmwareVersion"),
          const SizedBox(width: 24),
          const _FooterItem(icon: Icons.terminal, label: "PROTOCOL: V1.0"),
          const Spacer(),
          const _FooterItem(label: "SERIAL: OT-A1B2C3D4"),
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
                color: isSelected ? OpenTokenTheme.primary : Colors.white38,
                size: 20,
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Text(
                  label,
                  style: GoogleFonts.inter(
                    color: isSelected ? Colors.white : Colors.white38,
                    fontSize: 14,
                    fontWeight:
                        isSelected ? FontWeight.bold : FontWeight.normal,
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              if (isSelected)
                Container(
                  width: 4,
                  height: 4,
                  margin: const EdgeInsets.only(left: 8),
                  decoration: BoxDecoration(
                      color: OpenTokenTheme.primary, shape: BoxShape.circle),
                ),
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
