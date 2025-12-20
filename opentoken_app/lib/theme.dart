import 'package:flutter/material.dart';

class OpenTokenTheme {
  // Brand Colors
  static const Color electricPurple = Color(0xFFBC00FF);
  static const Color cyanPulse = Color(0xFF00F0FF);
  static const Color deepBackground = Color(0xFF050505);
  static const Color surfaceCard = Color(0xFF121214);
  static const Color textMain = Colors.white;
  static const Color textSecondary = Colors.white54;

  // Gradients
  static const LinearGradient brandGradient = LinearGradient(
    colors: [electricPurple, cyanPulse],
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
  );

  // Borders
  static final Border cardBorder = Border.all(
    color: Colors.white.withOpacity(0.08),
    width: 1,
  );

  // Shadow
  static final List<BoxShadow> softShadow = [
    BoxShadow(
      color: Colors.black.withOpacity(0.5),
      blurRadius: 20,
      offset: const Offset(0, 10),
    ),
  ];
}
