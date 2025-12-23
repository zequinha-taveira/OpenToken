import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:google_fonts/google_fonts.dart';
import '../theme.dart';

/// Credential Card - PRD Section 3.1
/// Displays a TOTP/HOTP credential with code, timer, and service info
class CredentialCard extends StatelessWidget {
  final String name;
  final String issuer;
  final String code;
  final double progress; // 0.0 to 1.0, where 1.0 = full time remaining
  final String algorithm;
  final int digits;
  final String type; // TOTP or HOTP
  final VoidCallback? onTap;
  final VoidCallback? onCopy;

  const CredentialCard({
    super.key,
    required this.name,
    this.issuer = '',
    required this.code,
    required this.progress,
    this.algorithm = 'SHA1',
    this.digits = 6,
    this.type = 'TOTP',
    this.onTap,
    this.onCopy,
  });

  @override
  Widget build(BuildContext context) {
    final serviceColor =
        OpenTokenTheme.getServiceColor(issuer.isNotEmpty ? issuer : name);
    final timeRemaining = (progress * 30).toInt();
    final isLow = progress < 0.2;

    return Container(
      margin: const EdgeInsets.only(bottom: OpenTokenTheme.space3),
      decoration: BoxDecoration(
        borderRadius: OpenTokenTheme.borderRadiusLg,
        gradient: OpenTokenTheme.cardGradient,
        border: Border.all(
          color: Colors.white.withOpacity(0.08),
          width: 1,
        ),
      ),
      child: ClipRRect(
        borderRadius: OpenTokenTheme.borderRadiusLg,
        child: BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
          child: Material(
            color: Colors.transparent,
            child: InkWell(
              onTap: onTap,
              borderRadius: OpenTokenTheme.borderRadiusLg,
              child: Padding(
                padding: const EdgeInsets.all(OpenTokenTheme.space4),
                child: Row(
                  children: [
                    // Service Icon
                    Container(
                      width: 48,
                      height: 48,
                      decoration: BoxDecoration(
                        color: serviceColor.withOpacity(0.15),
                        borderRadius: OpenTokenTheme.borderRadiusMd,
                      ),
                      child: Center(
                        child: Text(
                          _getServiceEmoji(issuer.isNotEmpty ? issuer : name),
                          style: const TextStyle(fontSize: 24),
                        ),
                      ),
                    ),
                    const SizedBox(width: OpenTokenTheme.space4),

                    // Name & Info
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            issuer.isNotEmpty ? issuer : name.split(':').first,
                            style: GoogleFonts.inter(
                              fontSize: OpenTokenTheme.textBase,
                              fontWeight: OpenTokenTheme.fontSemibold,
                              color: Colors.white,
                            ),
                          ),
                          const SizedBox(height: 4),
                          Row(
                            children: [
                              Text(
                                type,
                                style: GoogleFonts.inter(
                                  fontSize: OpenTokenTheme.textXs,
                                  color: Colors.white38,
                                ),
                              ),
                              const SizedBox(width: 8),
                              Container(
                                width: 4,
                                height: 4,
                                decoration: BoxDecoration(
                                  color: Colors.white24,
                                  borderRadius: BorderRadius.circular(2),
                                ),
                              ),
                              const SizedBox(width: 8),
                              Text(
                                algorithm,
                                style: GoogleFonts.inter(
                                  fontSize: OpenTokenTheme.textXs,
                                  color: Colors.white38,
                                ),
                              ),
                              const SizedBox(width: 8),
                              Container(
                                width: 4,
                                height: 4,
                                decoration: BoxDecoration(
                                  color: Colors.white24,
                                  borderRadius: BorderRadius.circular(2),
                                ),
                              ),
                              const SizedBox(width: 8),
                              Text(
                                "$digits digits",
                                style: GoogleFonts.inter(
                                  fontSize: OpenTokenTheme.textXs,
                                  color: Colors.white38,
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),

                    // Code Display
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.end,
                      children: [
                        GestureDetector(
                          onTap: () {
                            Clipboard.setData(ClipboardData(text: code));
                            onCopy?.call();
                          },
                          child: Text(
                            _formatCode(code),
                            style: GoogleFonts.jetBrainsMono(
                              fontSize: OpenTokenTheme.textXl,
                              fontWeight: OpenTokenTheme.fontSemibold,
                              letterSpacing: 4,
                              color: isLow
                                  ? OpenTokenTheme.warning
                                  : OpenTokenTheme.primary,
                            ),
                          ),
                        ),
                        const SizedBox(height: 4),
                        Row(
                          children: [
                            Icon(
                              Icons.timer_outlined,
                              size: 12,
                              color: isLow
                                  ? OpenTokenTheme.warning
                                  : Colors.white38,
                            ),
                            const SizedBox(width: 4),
                            Text(
                              "${timeRemaining}s",
                              style: GoogleFonts.inter(
                                fontSize: OpenTokenTheme.textXs,
                                color: isLow
                                    ? OpenTokenTheme.warning
                                    : Colors.white38,
                                fontWeight: OpenTokenTheme.fontMedium,
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),

                    const SizedBox(width: OpenTokenTheme.space3),

                    // Progress Indicator
                    SizedBox(
                      width: 36,
                      height: 36,
                      child: CircularProgressIndicator(
                        value: progress,
                        strokeWidth: 3,
                        backgroundColor: Colors.white.withOpacity(0.1),
                        color: isLow
                            ? OpenTokenTheme.warning
                            : OpenTokenTheme.primary,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
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

  String _getServiceEmoji(String serviceName) {
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

// Legacy support - keep PremiumCard as alias
@Deprecated('Use CredentialCard instead')
class PremiumCard extends CredentialCard {
  const PremiumCard({
    super.key,
    required String title,
    required String subtitle,
    required String code,
    required double progress,
    VoidCallback? onTap,
  }) : super(
          name: title,
          issuer: subtitle,
          code: code,
          progress: progress,
          onTap: onTap,
        );
}
