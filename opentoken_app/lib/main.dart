import 'package:flutter/material.dart';
import 'package:opentoken_dart/opentoken_dart.dart';

void main() {
  runApp(const OpenTokenApp());
}

class OpenTokenApp extends StatelessWidget {
  const OpenTokenApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'OpenToken Pro',
      theme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: const Color(0xFF007ACC),
        scaffoldBackgroundColor: const Color(0xFF121212),
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('OpenToken Pro', style: TextStyle(fontWeight: FontWeight.bold)),
        backgroundColor: Colors.transparent,
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.add_circle_outline, color: Color(0xFF007ACC)),
            onPressed: () {}, // Implementation of Add Account
          ),
        ],
      ),
      body: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: 3, # Mock for demonstration
        itemBuilder: (context, index) {
          return Card(
            color: const Color(0xFF1E1E1E),
            margin: const EdgeInsets.only(bottom: 12),
            child: ListTile(
              title: Text('Account $index', style: const TextStyle(color: Colors.white70)),
              subtitle: const Text('TOTP', style: TextStyle(color: Color(0xFF007ACC))),
              trailing: const Text('123 456', style: TextStyle(
                fontSize: 24, fontWeight: FontWeight.bold, color: Colors.white
              )),
            ),
          );
        },
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () {},
        backgroundColor: const Color(0xFF007ACC),
        child: const Icon(Icons.refresh),
      ),
    );
  }
}
