import 'package:flutter/material.dart';

/// OpenToken Design System
/// Based on the definitive PRD specifications
///
/// This theme provides a professional, security-focused UI
/// following the principle: "App provides UX, Hardware provides security"
class OpenTokenTheme {
  // ==========================================================================
  // PRIMARY COLORS - Trust & Professionalism
  // ==========================================================================

  /// Primary Blue - Represents trust and reliability
  static const Color primary = Color(0xFF0066FF);
  static const Color primaryDark = Color(0xFF0047CC);
  static const Color primaryLight = Color(0xFF3385FF);

  // ==========================================================================
  // SECONDARY COLORS - Security & Success
  // ==========================================================================

  /// Secondary Green - Represents security and verification
  static const Color secondary = Color(0xFF00CC88);
  static const Color secondaryDark = Color(0xFF009966);
  static const Color secondaryLight = Color(0xFF33DD99);

  // ==========================================================================
  // SEMANTIC COLORS
  // ==========================================================================

  static const Color success = Color(0xFF00CC88);
  static const Color warning = Color(0xFFFFB020);
  static const Color error = Color(0xFFFF3B30);
  static const Color info = Color(0xFF0066FF);

  // ==========================================================================
  // NEUTRAL PALETTE (Dark Theme)
  // ==========================================================================

  static const Color gray900 = Color(0xFF1A1A1A); // Text primary
  static const Color gray700 = Color(0xFF4A4A4A); // Text secondary
  static const Color gray500 = Color(0xFF888888); // Text disabled
  static const Color gray300 = Color(0xFFD1D1D1); // Borders (light mode)
  static const Color gray100 = Color(0xFFF5F5F5); // Background (light mode)
  static const Color white = Color(0xFFFFFFFF);

  // Dark mode specific
  static const Color deepBackground = Color(0xFF0A0A0C);
  static const Color surfaceCard = Color(0xFF121214);
  static const Color surfaceElevated = Color(0xFF1A1A1E);

  // ==========================================================================
  // GRADIENT
  // ==========================================================================

  static const LinearGradient brandGradient = LinearGradient(
    colors: [primary, secondary],
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
  );

  static LinearGradient cardGradient = LinearGradient(
    colors: [
      Colors.white.withOpacity(0.08),
      Colors.white.withOpacity(0.02),
    ],
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
  );

  // ==========================================================================
  // SPACING SYSTEM (based on 4px grid)
  // ==========================================================================

  static const double space1 = 4;
  static const double space2 = 8;
  static const double space3 = 12;
  static const double space4 = 16;
  static const double space5 = 24;
  static const double space6 = 32;
  static const double space7 = 48;
  static const double space8 = 64;
  static const double space9 = 96;

  // ==========================================================================
  // TYPOGRAPHY
  // ==========================================================================

  // Font sizes
  static const double textXs = 12; // Caption
  static const double textSm = 14; // Body small
  static const double textBase = 16; // Body
  static const double textLg = 18; // Lead
  static const double textXl = 24; // H3
  static const double text2Xl = 32; // H2
  static const double text3Xl = 48; // H1

  // Font weights
  static const FontWeight fontNormal = FontWeight.w400;
  static const FontWeight fontMedium = FontWeight.w500;
  static const FontWeight fontSemibold = FontWeight.w600;
  static const FontWeight fontBold = FontWeight.w700;

  // ==========================================================================
  // BORDER RADIUS
  // ==========================================================================

  static const double radiusSm = 4;
  static const double radiusMd = 8;
  static const double radiusLg = 12;
  static const double radiusXl = 16;
  static const double radiusFull = 9999;

  static BorderRadius borderRadiusSm = BorderRadius.circular(radiusSm);
  static BorderRadius borderRadiusMd = BorderRadius.circular(radiusMd);
  static BorderRadius borderRadiusLg = BorderRadius.circular(radiusLg);
  static BorderRadius borderRadiusXl = BorderRadius.circular(radiusXl);

  // ==========================================================================
  // SHADOWS
  // ==========================================================================

  static List<BoxShadow> shadowSm = [
    BoxShadow(
      color: Colors.black.withOpacity(0.05),
      blurRadius: 2,
      offset: const Offset(0, 1),
    ),
  ];

  static List<BoxShadow> shadowMd = [
    BoxShadow(
      color: Colors.black.withOpacity(0.07),
      blurRadius: 6,
      offset: const Offset(0, 4),
    ),
  ];

  static List<BoxShadow> shadowLg = [
    BoxShadow(
      color: Colors.black.withOpacity(0.1),
      blurRadius: 15,
      offset: const Offset(0, 10),
    ),
  ];

  static List<BoxShadow> shadowXl = [
    BoxShadow(
      color: Colors.black.withOpacity(0.15),
      blurRadius: 25,
      offset: const Offset(0, 20),
    ),
  ];

  // Glow effects for primary elements
  static List<BoxShadow> primaryGlow = [
    BoxShadow(
      color: primary.withOpacity(0.3),
      blurRadius: 20,
      offset: const Offset(0, 4),
    ),
  ];

  static List<BoxShadow> successGlow = [
    BoxShadow(
      color: success.withOpacity(0.3),
      blurRadius: 20,
      offset: const Offset(0, 4),
    ),
  ];

  // ==========================================================================
  // BORDERS
  // ==========================================================================

  static Border cardBorder = Border.all(
    color: Colors.white.withOpacity(0.08),
    width: 1,
  );

  static Border inputBorder = Border.all(
    color: Colors.white.withOpacity(0.1),
    width: 1,
  );

  static Border focusBorder = Border.all(
    color: primary,
    width: 2,
  );

  // ==========================================================================
  // BUTTON STYLES
  // ==========================================================================

  static ButtonStyle primaryButton = ElevatedButton.styleFrom(
    backgroundColor: primary,
    foregroundColor: white,
    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
    shape: RoundedRectangleBorder(
      borderRadius: borderRadiusMd,
    ),
    elevation: 0,
  );

  static ButtonStyle secondaryButton = ElevatedButton.styleFrom(
    backgroundColor: gray100,
    foregroundColor: gray900,
    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
    shape: RoundedRectangleBorder(
      borderRadius: borderRadiusMd,
    ),
    elevation: 0,
  );

  static ButtonStyle ghostButton = TextButton.styleFrom(
    foregroundColor: primary,
    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
    shape: RoundedRectangleBorder(
      borderRadius: borderRadiusMd,
    ),
  );

  static ButtonStyle dangerButton = ElevatedButton.styleFrom(
    backgroundColor: error.withOpacity(0.1),
    foregroundColor: error,
    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
    shape: RoundedRectangleBorder(
      borderRadius: borderRadiusMd,
      side: BorderSide(color: error.withOpacity(0.3)),
    ),
    elevation: 0,
  );

  // ==========================================================================
  // THEME DATA (for MaterialApp)
  // ==========================================================================

  static ThemeData get darkTheme => ThemeData(
        brightness: Brightness.dark,
        primaryColor: primary,
        scaffoldBackgroundColor: deepBackground,
        colorScheme: const ColorScheme.dark(
          primary: primary,
          secondary: secondary,
          surface: surfaceCard,
          error: error,
        ),
        fontFamily: 'Inter',
        useMaterial3: true,

        // AppBar
        appBarTheme: const AppBarTheme(
          backgroundColor: deepBackground,
          foregroundColor: white,
          elevation: 0,
        ),

        // Cards
        cardTheme: CardThemeData(
          color: surfaceCard,
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: borderRadiusLg,
            side: BorderSide(color: Colors.white.withOpacity(0.08)),
          ),
        ),

        // Input decoration
        inputDecorationTheme: InputDecorationTheme(
          filled: true,
          fillColor: Colors.white.withOpacity(0.03),
          border: OutlineInputBorder(
            borderRadius: borderRadiusMd,
            borderSide: BorderSide(color: Colors.white.withOpacity(0.1)),
          ),
          enabledBorder: OutlineInputBorder(
            borderRadius: borderRadiusMd,
            borderSide: BorderSide(color: Colors.white.withOpacity(0.1)),
          ),
          focusedBorder: OutlineInputBorder(
            borderRadius: borderRadiusMd,
            borderSide: const BorderSide(color: primary, width: 2),
          ),
          contentPadding:
              const EdgeInsets.symmetric(horizontal: 16, vertical: 16),
        ),

        // Elevated Button
        elevatedButtonTheme: ElevatedButtonThemeData(
          style: primaryButton,
        ),

        // Text Button
        textButtonTheme: TextButtonThemeData(
          style: ghostButton,
        ),

        // Divider
        dividerTheme: DividerThemeData(
          color: Colors.white.withOpacity(0.08),
          thickness: 1,
        ),
      );

  // ==========================================================================
  // SERVICE ICON COLORS (for credential cards)
  // ==========================================================================

  static const Map<String, Color> serviceColors = {
    'github': Color(0xFF2196F3), // Blue
    'google': Color(0xFFEA4335), // Red
    'gmail': Color(0xFFEA4335), // Red
    'aws': Color(0xFFFF9900), // Orange
    'amazon': Color(0xFFFF9900), // Orange
    'discord': Color(0xFF5865F2), // Purple
    'microsoft': Color(0xFF00A4EF), // Blue
    'dropbox': Color(0xFF0061FF), // Blue
    'twitter': Color(0xFF1DA1F2), // Blue
    'facebook': Color(0xFF1877F2), // Blue
    'slack': Color(0xFF4A154B), // Purple
    'proton': Color(0xFF6D4AFF), // Purple
  };

  /// Get color for a service name (case-insensitive, partial match)
  static Color getServiceColor(String serviceName) {
    final lower = serviceName.toLowerCase();
    for (final entry in serviceColors.entries) {
      if (lower.contains(entry.key)) {
        return entry.value;
      }
    }
    return primary; // Default to primary blue
  }
}
