import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme.dart';

/// Code Detail View - PRD Section 3.4
/// Full-screen view for a single credential with large code display
class CodeDetailView extends StatefulWidget {
  final String name;
  final String issuer;
  final String type;
  final String algorithm;
  final int digits;
  final int period;
  final int slotId;
  final DateTime created;
  final DateTime? lastUsed;
  final bool touchRequired;
  final Future<String> Function() onCalculate;
  final VoidCallback? onRename;
  final VoidCallback? onDelete;
  final VoidCallback? onBack;

  const CodeDetailView({
    super.key,
    required this.name,
    this.issuer = '',
    this.type = 'TOTP',
    this.algorithm = 'SHA1',
    this.digits = 6,
    this.period = 30,
    this.slotId = 1,
    required this.created,
    this.lastUsed,
    this.touchRequired = false,
    required this.onCalculate,
    this.onRename,
    this.onDelete,
    this.onBack,
  });

  @override
  State<CodeDetailView> createState() => _CodeDetailViewState();
}

class _CodeDetailViewState extends State<CodeDetailView> {
  String _code = '------';
  double _progress = 1.0;
  Timer? _timer;
  bool _copied = false;

  @override
  void initState() {
    super.initState();
    _fetchCode();
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
        _progress = 1.0 - ((now % widget.period) / widget.period);
      });
      if (now % widget.period == 0) {
        _fetchCode();
      }
    });
  }

  Future<void> _fetchCode() async {
    final code = await widget.onCalculate();
    setState(() => _code = code);
  }

  void _copyCode() {
    Clipboard.setData(ClipboardData(text: _code));
    setState(() => _copied = true);
    Future.delayed(const Duration(seconds: 2), () {
      if (mounted) setState(() => _copied = false);
    });
  }

  @override
  Widget build(BuildContext context) {
    final timeRemaining = (_progress * widget.period).toInt();
    final isLow = _progress < 0.2;
    final serviceColor = OpenTokenTheme.getServiceColor(
        widget.issuer.isNotEmpty ? widget.issuer : widget.name);

    return Scaffold(
      backgroundColor: OpenTokenTheme.deepBackground,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: widget.onBack ?? () => Navigator.pop(context),
        ),
        actions: [
          PopupMenuButton<String>(
            icon: const Icon(Icons.more_vert),
            color: OpenTokenTheme.surfaceCard,
            onSelected: (value) {
              if (value == 'rename') widget.onRename?.call();
              if (value == 'delete') widget.onDelete?.call();
            },
            itemBuilder: (context) => [
              const PopupMenuItem(
                value: 'rename',
                child: Row(
                  children: [
                    Icon(Icons.edit, size: 18),
                    SizedBox(width: 12),
                    Text('Rename'),
                  ],
                ),
              ),
              const PopupMenuItem(
                value: 'delete',
                child: Row(
                  children: [
                    Icon(Icons.delete, size: 18, color: Colors.redAccent),
                    SizedBox(width: 12),
                    Text('Delete', style: TextStyle(color: Colors.redAccent)),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(OpenTokenTheme.space5),
        child: Column(
          children: [
            // ═══════════════════════════════════════════════════════════════════
            // CODE DISPLAY CARD
            // ═══════════════════════════════════════════════════════════════════
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(OpenTokenTheme.space7),
              decoration: BoxDecoration(
                gradient: OpenTokenTheme.cardGradient,
                borderRadius: OpenTokenTheme.borderRadiusXl,
                border: OpenTokenTheme.cardBorder,
              ),
              child: Column(
                children: [
                  // Service icon & name
                  Container(
                    width: 64,
                    height: 64,
                    decoration: BoxDecoration(
                      color: serviceColor.withOpacity(0.15),
                      borderRadius: OpenTokenTheme.borderRadiusLg,
                    ),
                    child: Center(
                      child: Text(
                        _getServiceEmoji(),
                        style: const TextStyle(fontSize: 32),
                      ),
                    ),
                  ),
                  const SizedBox(height: OpenTokenTheme.space4),
                  Text(
                    widget.issuer.isNotEmpty ? widget.issuer : widget.name,
                    style: GoogleFonts.inter(
                      color: Colors.white,
                      fontSize: OpenTokenTheme.textXl,
                      fontWeight: OpenTokenTheme.fontBold,
                    ),
                  ),
                  if (widget.issuer.isNotEmpty)
                    Text(
                      widget.name,
                      style: GoogleFonts.inter(
                        color: Colors.white54,
                        fontSize: OpenTokenTheme.textSm,
                      ),
                    ),

                  const SizedBox(height: OpenTokenTheme.space7),

                  // Large code display
                  Text(
                    _formatCode(_code),
                    style: GoogleFonts.jetBrainsMono(
                      fontSize: 56,
                      fontWeight: OpenTokenTheme.fontBold,
                      letterSpacing: 8,
                      color: isLow
                          ? OpenTokenTheme.warning
                          : OpenTokenTheme.primary,
                    ),
                  ),

                  const SizedBox(height: OpenTokenTheme.space5),

                  // Timer display
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(
                        Icons.timer_outlined,
                        size: 18,
                        color: isLow ? OpenTokenTheme.warning : Colors.white54,
                      ),
                      const SizedBox(width: 8),
                      Text(
                        "Valid for $timeRemaining seconds",
                        style: GoogleFonts.inter(
                          color:
                              isLow ? OpenTokenTheme.warning : Colors.white54,
                          fontSize: OpenTokenTheme.textSm,
                        ),
                      ),
                    ],
                  ),

                  const SizedBox(height: OpenTokenTheme.space3),

                  // Progress bar
                  SizedBox(
                    width: 200,
                    child: ClipRRect(
                      borderRadius: BorderRadius.circular(4),
                      child: LinearProgressIndicator(
                        value: _progress,
                        backgroundColor: Colors.white.withOpacity(0.1),
                        valueColor: AlwaysStoppedAnimation<Color>(
                          isLow
                              ? OpenTokenTheme.warning
                              : OpenTokenTheme.primary,
                        ),
                        minHeight: 6,
                      ),
                    ),
                  ),

                  const SizedBox(height: OpenTokenTheme.space6),

                  // Copy button
                  SizedBox(
                    width: 200,
                    child: ElevatedButton.icon(
                      onPressed: _copyCode,
                      icon: Icon(_copied ? Icons.check : Icons.copy, size: 18),
                      label: Text(_copied ? "Copied!" : "Copy Code"),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: _copied
                            ? OpenTokenTheme.secondary
                            : OpenTokenTheme.primary,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(
                            horizontal: OpenTokenTheme.space5,
                            vertical: OpenTokenTheme.space3),
                        shape: RoundedRectangleBorder(
                            borderRadius: OpenTokenTheme.borderRadiusMd),
                      ),
                    ),
                  ),
                ],
              ),
            ),

            const SizedBox(height: OpenTokenTheme.space6),

            // ═══════════════════════════════════════════════════════════════════
            // DETAILS SECTION
            // ═══════════════════════════════════════════════════════════════════
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(OpenTokenTheme.space5),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.02),
                borderRadius: OpenTokenTheme.borderRadiusLg,
                border: Border.all(color: Colors.white.withOpacity(0.05)),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    "Details",
                    style: GoogleFonts.inter(
                      color: Colors.white,
                      fontSize: OpenTokenTheme.textBase,
                      fontWeight: OpenTokenTheme.fontSemibold,
                    ),
                  ),
                  const SizedBox(height: OpenTokenTheme.space4),
                  const Divider(color: Colors.white10),
                  const SizedBox(height: OpenTokenTheme.space3),
                  _buildDetailRow("Type",
                      "${widget.type} (${widget.type == 'TOTP' ? 'Time-based' : 'Counter-based'})"),
                  _buildDetailRow("Algorithm", widget.algorithm),
                  _buildDetailRow("Digits", widget.digits.toString()),
                  _buildDetailRow("Period", "${widget.period} seconds"),
                  _buildDetailRow("Slot", "#${widget.slotId}"),
                  _buildDetailRow("Created", _formatDate(widget.created)),
                  _buildDetailRow(
                      "Last used",
                      widget.lastUsed != null
                          ? _formatDate(widget.lastUsed!)
                          : "Never"),
                  _buildDetailRow(
                      "Touch required", widget.touchRequired ? "Yes" : "No"),
                ],
              ),
            ),

            const SizedBox(height: OpenTokenTheme.space5),

            // ═══════════════════════════════════════════════════════════════════
            // ACTIONS
            // ═══════════════════════════════════════════════════════════════════
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: widget.onRename,
                    icon: const Icon(Icons.edit, size: 18),
                    label: const Text("Rename"),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: Colors.white,
                      side: BorderSide(color: Colors.white.withOpacity(0.1)),
                      padding: const EdgeInsets.symmetric(
                          horizontal: OpenTokenTheme.space5,
                          vertical: OpenTokenTheme.space4),
                      shape: RoundedRectangleBorder(
                          borderRadius: OpenTokenTheme.borderRadiusMd),
                    ),
                  ),
                ),
                const SizedBox(width: OpenTokenTheme.space3),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: widget.onDelete,
                    icon: const Icon(Icons.delete,
                        size: 18, color: OpenTokenTheme.error),
                    label: const Text("Delete",
                        style: TextStyle(color: OpenTokenTheme.error)),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: OpenTokenTheme.error,
                      side: BorderSide(
                          color: OpenTokenTheme.error.withOpacity(0.3)),
                      padding: const EdgeInsets.symmetric(
                          horizontal: OpenTokenTheme.space5,
                          vertical: OpenTokenTheme.space4),
                      shape: RoundedRectangleBorder(
                          borderRadius: OpenTokenTheme.borderRadiusMd),
                    ),
                  ),
                ),
              ],
            ),

            const SizedBox(height: OpenTokenTheme.space7),
          ],
        ),
      ),
    );
  }

  Widget _buildDetailRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
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

  String _formatCode(String code) {
    if (code.length == 6) {
      return '${code.substring(0, 3)} ${code.substring(3)}';
    } else if (code.length == 8) {
      return '${code.substring(0, 4)} ${code.substring(4)}';
    }
    return code;
  }

  String _formatDate(DateTime date) {
    return '${date.year}-${date.month.toString().padLeft(2, '0')}-${date.day.toString().padLeft(2, '0')} '
        '${date.hour.toString().padLeft(2, '0')}:${date.minute.toString().padLeft(2, '0')}';
  }

  String _getServiceEmoji() {
    final serviceName = widget.issuer.isNotEmpty ? widget.issuer : widget.name;
    final lower = serviceName.toLowerCase();
    if (lower.contains('github')) return '🐙';
    if (lower.contains('google') || lower.contains('gmail')) return '📧';
    if (lower.contains('aws') || lower.contains('amazon')) return '☁️';
    if (lower.contains('discord')) return '💬';
    if (lower.contains('microsoft')) return '🪟';
    if (lower.contains('dropbox')) return '📦';
    if (lower.contains('twitter') || lower.contains('x.com')) return '🐦';
    if (lower.contains('facebook') || lower.contains('meta')) return '👤';
    if (lower.contains('slack')) return '💼';
    if (lower.contains('proton')) return '🔒';
    return '🔐';
  }
}
