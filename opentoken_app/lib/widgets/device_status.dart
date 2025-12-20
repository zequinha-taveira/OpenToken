import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class DeviceStatusBanner extends StatelessWidget {
  final bool isConnected;
  final String transportName;

  const DeviceStatusBanner({
    super.key,
    required this.isConnected,
    required this.transportName,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: isConnected
            ? const Color(0xFF00A3FF).withOpacity(0.1)
            : Colors.redAccent.withOpacity(0.1),
        border: Border(
          bottom: BorderSide(
            color: isConnected
                ? const Color(0xFF00A3FF).withOpacity(0.2)
                : Colors.redAccent.withOpacity(0.2),
          ),
        ),
      ),
      child: Row(
        children: [
          Icon(
            isConnected ? Icons.usb : Icons.usb_off,
            size: 16,
            color: isConnected ? const Color(0xFF00A3FF) : Colors.redAccent,
          ),
          const SizedBox(width: 8),
          Text(
            isConnected
                ? 'Connected via $transportName'
                : 'Device Disconnected',
            style: GoogleFonts.inter(
              fontSize: 12,
              fontWeight: FontWeight.w500,
              color: isConnected ? const Color(0xFF00A3FF) : Colors.redAccent,
            ),
          ),
          const Spacer(),
          if (isConnected)
            Container(
              width: 8,
              height: 8,
              decoration: const BoxDecoration(
                color: Color(0xFF00A3FF),
                shape: BoxShape.circle,
                boxShadow: [
                  BoxShadow(
                    color: Color(0xFF00A3FF),
                    blurRadius: 4,
                    spreadRadius: 1,
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }
}
