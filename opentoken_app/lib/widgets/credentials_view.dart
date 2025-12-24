import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme.dart';
import 'premium_card.dart';

/// Credentials View - PRD Section 3.1 (Home Screen)
/// Displays device status and list of OATH credentials
class CredentialsView extends StatefulWidget {
  final List<Map<String, String>> accounts;
  final Map<String, String> codes;
  final double progress;
  final VoidCallback onAdd;
  final bool isConnected;
  final String serial;
  final String firmwareVersion;
  final int slotsUsed;
  final int slotsTotal;
  final bool isRefreshing;
  final VoidCallback onRefresh;

  const CredentialsView({
    super.key,
    required this.accounts,
    required this.codes,
    required this.progress,
    required this.onAdd,
    this.isConnected = true,
    this.serial = 'OT-A1B2C3D4E5F6',
    this.firmwareVersion = '1.0.0',
    this.slotsUsed = 5,
    this.slotsTotal = 200,
    this.isRefreshing = false,
    required this.onRefresh,
  });

  @override
  State<CredentialsView> createState() => _CredentialsViewState();
}

class _CredentialsViewState extends State<CredentialsView> {
  final TextEditingController _searchController = TextEditingController();
  String _filter = "";

  @override
  Widget build(BuildContext context) {
    final filteredAccounts = widget.accounts.where((acc) {
      final name = acc["name"]?.toLowerCase() ?? "";
      return name.contains(_filter.toLowerCase());
    }).toList();

    return Column(
      children: [
        // ═════════════════════════════════════════════════════════════════════
        // HEADER & SEARCH
        // ═════════════════════════════════════════════════════════════════════
        Padding(
          padding: const EdgeInsets.all(OpenTokenTheme.space5),
          child: Row(
            children: [
              Expanded(
                child: Container(
                  height: 48,
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.05),
                    borderRadius: OpenTokenTheme.borderRadiusLg,
                    border: Border.all(color: Colors.white.withOpacity(0.1)),
                  ),
                  child: Row(
                    children: [
                      const SizedBox(width: OpenTokenTheme.space4),
                      Icon(Icons.search,
                          color: Colors.white.withOpacity(0.3), size: 20),
                      const SizedBox(width: OpenTokenTheme.space3),
                      Expanded(
                        child: TextField(
                          controller: _searchController,
                          onChanged: (val) => setState(() => _filter = val),
                          style: GoogleFonts.inter(
                              color: Colors.white,
                              fontSize: OpenTokenTheme.textSm),
                          decoration: InputDecoration(
                            hintText: "Search credentials...",
                            hintStyle: GoogleFonts.inter(
                                color: Colors.white.withOpacity(0.2)),
                            border: InputBorder.none,
                            isDense: true,
                            contentPadding: EdgeInsets.zero,
                          ),
                        ),
                      ),
                      Container(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 8, vertical: 4),
                        margin: const EdgeInsets.only(right: 8),
                        decoration: BoxDecoration(
                          color: Colors.white.withOpacity(0.05),
                          borderRadius: BorderRadius.circular(4),
                        ),
                        child: Text("⌘K",
                            style: GoogleFonts.inter(
                                color: Colors.white.withOpacity(0.3),
                                fontSize: 10,
                                fontWeight: FontWeight.bold)),
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(width: OpenTokenTheme.space4),
              _ActionButton(
                icon: Icons.refresh,
                onPressed: widget.isRefreshing ? null : widget.onRefresh,
                isLoading: widget.isRefreshing,
              ),
              const SizedBox(width: OpenTokenTheme.space3),
              ElevatedButton.icon(
                onPressed: widget.onAdd,
                icon: const Icon(Icons.add, size: 18),
                label: const Text("Add Credential"),
                style: ElevatedButton.styleFrom(
                  backgroundColor: OpenTokenTheme.primary,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(
                      horizontal: OpenTokenTheme.space5,
                      vertical: OpenTokenTheme.space4),
                  shape: RoundedRectangleBorder(
                      borderRadius: OpenTokenTheme.borderRadiusMd),
                  elevation: 0,
                ),
              ),
            ],
          ),
        ),

        Expanded(
          child: ListView(
            padding:
                const EdgeInsets.symmetric(horizontal: OpenTokenTheme.space5),
            children: [
              // ═════════════════════════════════════════════════════════════════
              // DEVICE STATUS CARD
              // ═════════════════════════════════════════════════════════════════
              _buildDeviceStatusCard(),

              const SizedBox(height: OpenTokenTheme.space6),

              // ═════════════════════════════════════════════════════════════════
              // CREDENTIALS HEADER
              // ═════════════════════════════════════════════════════════════════
              Row(
                children: [
                  const Icon(Icons.key, color: Colors.white54, size: 18),
                  const SizedBox(width: OpenTokenTheme.space2),
                  Text(
                    "Credentials (${widget.accounts.length})",
                    style: GoogleFonts.inter(
                      color: Colors.white,
                      fontSize: OpenTokenTheme.textBase,
                      fontWeight: OpenTokenTheme.fontSemibold,
                    ),
                  ),
                ],
              ),

              const SizedBox(height: OpenTokenTheme.space4),

              // ═════════════════════════════════════════════════════════════════
              // CREDENTIALS LIST
              // ═════════════════════════════════════════════════════════════════
              ...filteredAccounts.map((acc) {
                final name = acc["name"] ?? "";
                final parts = name.split(':');
                final issuer = parts.length > 1 ? parts[0] : "";
                final account = parts.length > 1 ? parts[1] : parts[0];

                return CredentialCard(
                  name: account,
                  issuer: issuer,
                  code: widget.codes[name] ?? "------",
                  progress: widget.progress,
                  algorithm: acc["algorithm"] ?? "SHA1",
                  digits: int.tryParse(acc["digits"] ?? "6") ?? 6,
                  type: acc["type"] ?? "TOTP",
                  onTap: () {
                    // TODO: Navigate to code detail view
                  },
                  onCopy: () {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                        content: Text('Code copied to clipboard'),
                        backgroundColor: OpenTokenTheme.surfaceElevated,
                        duration: Duration(seconds: 2),
                      ),
                    );
                  },
                );
              }),

              // Empty state
              if (filteredAccounts.isEmpty) _buildEmptyState(),

              const SizedBox(height: OpenTokenTheme.space7),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildDeviceStatusCard() {
    final slotPercentage =
        (widget.slotsUsed / widget.slotsTotal * 100).toStringAsFixed(1);

    return Container(
      padding: const EdgeInsets.all(OpenTokenTheme.space5),
      decoration: BoxDecoration(
        gradient: OpenTokenTheme.cardGradient,
        borderRadius: OpenTokenTheme.borderRadiusXl,
        border: OpenTokenTheme.cardBorder,
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Header row
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: OpenTokenTheme.primary.withOpacity(0.1),
                  borderRadius: OpenTokenTheme.borderRadiusMd,
                ),
                child: const Icon(
                  Icons.usb,
                  color: OpenTokenTheme.primary,
                  size: 24,
                ),
              ),
              const SizedBox(width: OpenTokenTheme.space4),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Text(
                          "OpenToken RP2350",
                          style: GoogleFonts.inter(
                            color: Colors.white,
                            fontWeight: OpenTokenTheme.fontBold,
                            fontSize: OpenTokenTheme.textLg,
                          ),
                        ),
                        const SizedBox(width: OpenTokenTheme.space3),
                        Container(
                          padding: const EdgeInsets.symmetric(
                              horizontal: 8, vertical: 4),
                          decoration: BoxDecoration(
                            color: widget.isConnected
                                ? OpenTokenTheme.secondary.withOpacity(0.15)
                                : OpenTokenTheme.error.withOpacity(0.15),
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Container(
                                width: 6,
                                height: 6,
                                decoration: BoxDecoration(
                                  color: widget.isConnected
                                      ? OpenTokenTheme.secondary
                                      : OpenTokenTheme.error,
                                  shape: BoxShape.circle,
                                ),
                              ),
                              const SizedBox(width: 6),
                              Text(
                                widget.isConnected
                                    ? "Connected"
                                    : "Disconnected",
                                style: GoogleFonts.inter(
                                  color: widget.isConnected
                                      ? OpenTokenTheme.secondary
                                      : OpenTokenTheme.error,
                                  fontSize: 11,
                                  fontWeight: OpenTokenTheme.fontMedium,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 4),
                    Text(
                      "Serial: ${widget.serial}",
                      style: GoogleFonts.jetBrainsMono(
                        color: Colors.white54,
                        fontSize: OpenTokenTheme.textSm,
                      ),
                    ),
                  ],
                ),
              ),
              Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: [
                  Text(
                    "v${widget.firmwareVersion}",
                    style: GoogleFonts.jetBrainsMono(
                      color: Colors.white,
                      fontSize: OpenTokenTheme.textSm,
                      fontWeight: OpenTokenTheme.fontMedium,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    "Firmware",
                    style: GoogleFonts.inter(
                      color: Colors.white38,
                      fontSize: OpenTokenTheme.textXs,
                    ),
                  ),
                ],
              ),
            ],
          ),

          const SizedBox(height: OpenTokenTheme.space5),

          // Storage bar
          Row(
            children: [
              const Icon(Icons.folder_outlined,
                  color: Colors.white38, size: 16),
              const SizedBox(width: OpenTokenTheme.space2),
              Text(
                "Storage: ${widget.slotsUsed} / ${widget.slotsTotal} slots used",
                style: GoogleFonts.inter(
                  color: Colors.white54,
                  fontSize: OpenTokenTheme.textSm,
                ),
              ),
              const Spacer(),
              Text(
                "$slotPercentage%",
                style: GoogleFonts.inter(
                  color: Colors.white38,
                  fontSize: OpenTokenTheme.textSm,
                ),
              ),
            ],
          ),
          const SizedBox(height: OpenTokenTheme.space2),
          ClipRRect(
            borderRadius: BorderRadius.circular(4),
            child: LinearProgressIndicator(
              value: widget.slotsUsed / widget.slotsTotal,
              backgroundColor: Colors.white.withOpacity(0.1),
              valueColor:
                  const AlwaysStoppedAnimation<Color>(OpenTokenTheme.primary),
              minHeight: 6,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyState() {
    return Container(
      padding: const EdgeInsets.all(OpenTokenTheme.space8),
      child: Column(
        children: [
          Icon(
            Icons.key_off,
            size: 64,
            color: Colors.white.withOpacity(0.1),
          ),
          const SizedBox(height: OpenTokenTheme.space5),
          Text(
            "No credentials found",
            style: GoogleFonts.inter(
              color: Colors.white54,
              fontSize: OpenTokenTheme.textLg,
              fontWeight: OpenTokenTheme.fontMedium,
            ),
          ),
          const SizedBox(height: OpenTokenTheme.space2),
          Text(
            "Add your first credential to get started",
            style: GoogleFonts.inter(
              color: Colors.white38,
              fontSize: OpenTokenTheme.textSm,
            ),
          ),
          const SizedBox(height: OpenTokenTheme.space5),
          ElevatedButton.icon(
            onPressed: widget.onAdd,
            icon: const Icon(Icons.add, size: 18),
            label: const Text("Add Credential"),
            style: ElevatedButton.styleFrom(
              backgroundColor: OpenTokenTheme.primary,
              foregroundColor: Colors.white,
              padding: const EdgeInsets.symmetric(
                  horizontal: OpenTokenTheme.space5,
                  vertical: OpenTokenTheme.space3),
              shape: RoundedRectangleBorder(
                  borderRadius: OpenTokenTheme.borderRadiusMd),
            ),
          ),
        ],
      ),
    );
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final VoidCallback? onPressed;
  final bool isLoading;

  const _ActionButton({
    required this.icon,
    required this.onPressed,
    this.isLoading = false,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: OpenTokenTheme.borderRadiusMd,
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: Material(
        color: Colors.transparent,
        child: IconButton(
          icon: isLoading
              ? const SizedBox(
                  width: 18,
                  height: 18,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: Colors.white70,
                  ),
                )
              : Icon(icon, color: Colors.white70, size: 20),
          onPressed: onPressed,
        ),
      ),
    );
  }
}
