// This is a basic Flutter widget test.
//
// To perform an interaction with a widget in your test, use the WidgetTester
// utility in the flutter_test package. For example, you can send tap and scroll
// gestures. You can also use WidgetTester to find child widgets in the widget
// tree, read text, and verify that the values of widget properties are correct.

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:opentoken_app/main.dart';

void main() {
  testWidgets('OpenToken App smoke test', (WidgetTester tester) async {
    // Set a larger surface size for the desktop-oriented UI
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;

    // Build our app and trigger a frame.
    await tester.pumpWidget(const OpenTokenApp());

    // Verify that our app starts and shows the main view.
    expect(find.text('OpenToken Authenticator'), findsOneWidget);
  });
}
