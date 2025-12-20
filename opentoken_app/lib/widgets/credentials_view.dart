import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'premium_card.dart';

class CredentialsView extends StatefulWidget {
  final List<Map<String, String>> accounts;
  final Map<String, String> codes;
  final double progress;
  final VoidCallback onAdd;

  const CredentialsView({
    super.key,
    required this.accounts,
    required this.codes,
    required this.progress,
    required this.onAdd,
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
        Padding(
          padding: const EdgeInsets.all(24.0),
          child: Row(
            children: [
              Expanded(
                child: Container(
                  height: 48,
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.05),
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: Colors.white.withOpacity(0.1)),
                  ),
                  child: Row(
                    children: [
                      const SizedBox(width: 16),
                      Icon(Icons.search,
                          color: Colors.white.withOpacity(0.3), size: 20),
                      const SizedBox(width: 12),
                      Expanded(
                        child: TextField(
                          controller: _searchController,
                          onChanged: (val) => setState(() => _filter = val),
                          style: GoogleFonts.inter(
                              color: Colors.white, fontSize: 14),
                          decoration: InputDecoration(
                            hintText: "Search credentials (cmd+k)",
                            hintStyle: GoogleFonts.inter(
                                color: Colors.white.withOpacity(0.2)),
                            border: InputBorder.none,
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
              const SizedBox(width: 16),
              _ActionButton(
                icon: Icons.refresh,
                onPressed: () {},
              ),
              const SizedBox(width: 12),
              ElevatedButton.icon(
                onPressed: widget.onAdd,
                icon: const Icon(Icons.add, size: 18),
                label: const Text("Add Credential"),
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF007BFF),
                  foregroundColor: Colors.white,
                  padding:
                      const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(8)),
                  elevation: 8,
                  shadowColor: Colors.blue.withOpacity(0.5),
                ),
              ),
            ],
          ),
        ),
        Expanded(
          child: ListView.builder(
            itemCount: filteredAccounts.length,
            padding: const EdgeInsets.symmetric(horizontal: 24),
            itemBuilder: (context, index) {
              final acc = filteredAccounts[index];
              return PremiumCard(
                title: acc["name"]!,
                subtitle: acc["type"]!,
                code: widget.codes[acc["name"]] ?? "******",
                progress: widget.progress,
                onTap: () {},
              );
            },
          ),
        ),
      ],
    );
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final VoidCallback onPressed;

  const _ActionButton({required this.icon, required this.onPressed});

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: IconButton(
        icon: Icon(icon, color: Colors.white70, size: 20),
        onPressed: onPressed,
      ),
    );
  }
}
