import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class DeviceStatusView extends StatelessWidget {
  final String firmwareVersion;
  final bool isConnected;
  final VoidCallback onReboot;

  const DeviceStatusView({
    super.key,
    required this.firmwareVersion,
    required this.isConnected,
    required this.onReboot,
  });

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      child: Padding(
        padding: const EdgeInsets.all(40.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Text(
                          "Status do Dispositivo",
                          style: GoogleFonts.inter(
                              fontSize: 32,
                              fontWeight: FontWeight.bold,
                              color: Colors.white),
                        ),
                        const SizedBox(width: 16),
                        Container(
                          padding: const EdgeInsets.symmetric(
                              horizontal: 8, vertical: 4),
                          decoration: BoxDecoration(
                            color: const Color(0xFF00F0FF).withOpacity(0.15),
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(
                                color:
                                    const Color(0xFF00F0FF).withOpacity(0.3)),
                          ),
                          child: Text("RP2350 NATIVE",
                              style: GoogleFonts.jetBrainsMono(
                                  color: const Color(0xFF00F0FF),
                                  fontSize: 10,
                                  fontWeight: FontWeight.bold)),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    Text(
                      "Alvo: Elemento Seguro OpenToken RP2350",
                      style: GoogleFonts.inter(
                          color: Colors.white54, fontSize: 14),
                    ),
                  ],
                ),
                ElevatedButton.icon(
                  onPressed: () {},
                  icon: const Icon(Icons.refresh, size: 18),
                  label: const Text("Verificar Atualizações"),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF007BFF),
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(
                        horizontal: 20, vertical: 16),
                    shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(8)),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 48),
            _buildHardwareCard(),
            const SizedBox(height: 32),
            _buildStorageBar(),
            const SizedBox(height: 48),
            _buildDangerZone(),
          ],
        ),
      ),
    );
  }

  Widget _buildHardwareCard() {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.05)),
      ),
      child: Row(
        children: [
          Container(
            width: 200,
            height: 200,
            margin: const EdgeInsets.all(24),
            decoration: BoxDecoration(
              color: Colors.black.withOpacity(0.2),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(color: Colors.white.withOpacity(0.05)),
            ),
            child: Center(
              child: Icon(Icons.developer_board,
                  color: Colors.white.withOpacity(0.1), size: 80),
            ),
          ),
          Expanded(
            child: Padding(
              padding: const EdgeInsets.only(right: 48),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Text("OpenToken RP2350",
                          style: GoogleFonts.inter(
                              fontSize: 24,
                              fontWeight: FontWeight.bold,
                              color: Colors.white)),
                      const SizedBox(width: 12),
                      Icon(
                        isConnected ? Icons.check_circle : Icons.error,
                        color:
                            isConnected ? const Color(0xFF00F0FF) : Colors.red,
                        size: 20,
                      ),
                    ],
                  ),
                  const SizedBox(height: 4),
                  Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                    decoration: BoxDecoration(
                        color: Colors.white.withOpacity(0.05),
                        borderRadius: BorderRadius.circular(4)),
                    child: Text("Hardware de Produção",
                        style: GoogleFonts.inter(
                            color: Colors.white38,
                            fontSize: 10,
                            fontWeight: FontWeight.bold)),
                  ),
                  const SizedBox(height: 32),
                  Row(
                    children: [
                      _buildInfoItem("NÚMERO DE SÉRIE", "OT-8821-X99-A01"),
                      const SizedBox(width: 64),
                      _buildInfoItem(
                          "ARQUITETURA MCU", "RP2350 (RISC-V / ARM)"),
                    ],
                  ),
                  const SizedBox(height: 24),
                  Row(
                    children: [
                      _buildInfoItem("VERSÃO DO FIRMWARE", firmwareVersion,
                          isLatest: true),
                      const SizedBox(width: 64),
                      _buildInfoItem(
                          "SUPORTE A PROTOCOLO", "FIDO2 / U2F / OTP / PIV"),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildInfoItem(String label, String value, {bool isLatest = false}) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label,
            style: GoogleFonts.inter(
                color: Colors.white38,
                fontSize: 10,
                fontWeight: FontWeight.bold,
                letterSpacing: 1.2)),
        const SizedBox(height: 4),
        Row(
          children: [
            Text(value,
                style: GoogleFonts.jetBrainsMono(
                    color: Colors.white,
                    fontSize: 14,
                    fontWeight: FontWeight.w500)),
            if (isLatest) ...[
              const SizedBox(width: 8),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
                decoration: BoxDecoration(
                    color: const Color(0xFF00F0FF).withOpacity(0.1),
                    borderRadius: BorderRadius.circular(4)),
                child: const Text("Mais Recente",
                    style: TextStyle(
                        color: Color(0xFF00F0FF),
                        fontSize: 8,
                        fontWeight: FontWeight.bold)),
              ),
            ],
          ],
        ),
      ],
    );
  }

  Widget _buildStorageBar() {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Row(
              children: [
                const Icon(Icons.storage, color: Colors.white38, size: 16),
                const SizedBox(width: 8),
                Text("Armazenamento Flash Seguro (Criptografado)",
                    style:
                        GoogleFonts.inter(color: Colors.white38, fontSize: 12)),
              ],
            ),
            Text("128KB / 2MB Utilizado",
                style: GoogleFonts.inter(color: Colors.white38, fontSize: 12)),
          ],
        ),
        const SizedBox(height: 12),
        ClipRRect(
          borderRadius: BorderRadius.circular(4),
          child: LinearProgressIndicator(
            value: 0.1,
            backgroundColor: Colors.white.withOpacity(0.05),
            valueColor: const AlwaysStoppedAnimation<Color>(Color(0xFF007BFF)),
            minHeight: 8,
          ),
        ),
      ],
    );
  }

  Widget _buildDangerZone() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(32),
      decoration: BoxDecoration(
        color: Colors.red.withOpacity(0.02),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.red.withOpacity(0.1)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.warning_amber_rounded,
                  color: Colors.redAccent, size: 24),
              const SizedBox(width: 12),
              Text("Zona de Perigo",
                  style: GoogleFonts.inter(
                      color: Colors.redAccent,
                      fontSize: 18,
                      fontWeight: FontWeight.bold)),
            ],
          ),
          const SizedBox(height: 32),
          _buildDangerItem(
            "Reset de Fábrica",
            "Esta ação apagará todos os segredos, chaves e dados de configuração do elemento seguro. Esta ação não pode ser desfeita.",
            "Limpar Dispositivo",
            Icons.delete_forever,
            () {},
          ),
          const Padding(
            padding: EdgeInsets.symmetric(vertical: 24.0),
            child: Divider(color: Colors.white10),
          ),
          _buildDangerItem(
            "Gravação Manual de Firmware",
            "Reinicia o dispositivo em modo BOOTSEL para permitir a gravação de um novo arquivo .uf2.",
            "Entrar em BOOTSEL",
            Icons.upload_file,
            onReboot,
          ),
        ],
      ),
    );
  }

  Widget _buildDangerItem(String title, String description, String buttonLabel,
      IconData icon, VoidCallback onPressed) {
    return Row(
      children: [
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(title,
                  style: GoogleFonts.inter(
                      color: Colors.white,
                      fontSize: 15,
                      fontWeight: FontWeight.bold)),
              const SizedBox(height: 4),
              Text(description,
                  style: GoogleFonts.inter(
                      color: Colors.white54, fontSize: 13, height: 1.5)),
            ],
          ),
        ),
        const SizedBox(width: 48),
        OutlinedButton.icon(
          onPressed: onPressed,
          icon: Icon(icon, size: 18),
          label: Text(buttonLabel),
          style: OutlinedButton.styleFrom(
            foregroundColor: Colors.white,
            side: BorderSide(color: Colors.white.withOpacity(0.1)),
            padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
          ),
        ),
      ],
    );
  }
}
