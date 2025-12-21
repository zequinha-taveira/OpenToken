# Estrutura de Repositório da Biblioteca OpenToken

Este documento descreve a organização completa do repositório OpenToken, um firmware de segurança universal para o microcontrolador Raspberry Pi RP2350 (Pico 2).

## 📁 Visão Geral da Estrutura

```
OpenToken/
├── 📂 assets/                    # Recursos visuais e mídia
├── 📂 boards/                    # Configurações de hardware específicas
├── 📂 docs/                      # Documentação do projeto
├── 📂 host_tools/                # Ferramentas e SDKs para desenvolvimento host
├── 📂 include/                   # Cabeçalhos (headers) do firmware
├── 📂 libraries/                 # Bibliotecas cliente para diferentes linguagens
├── 📂 opentoken_app/             # Aplicativo Flutter oficial (Desktop/Mobile)
├── 📂 src/                       # Código-fonte principal do firmware
├── 📄 CMakeLists.txt             # Configuração de build CMake
├── 📄 pico_sdk_init.cmake        # Inicialização do Raspberry Pi Pico SDK
├── 📄 LICENSE                    # Licença MIT
├── 📄 README.md                  # Documentação principal
└── 📄 CONTRIBUTING.md            # Guia de contribuição
```

---

## 📂 Descrição Detalhada dos Diretórios

### 🎨 `assets/`
**Propósito**: Recursos visuais e mídia do projeto.

```
assets/
└── logo.png                      # Logo oficial do OpenToken
```

---

### 🔧 `boards/`
**Propósito**: Configurações específicas de hardware e definições de board.

```
boards/
└── tenstar_rp2350.h              # Definições de hardware para Tenstar RP2350
```

**Conteúdo**:
- Definições de pinos GPIO
- Configurações de clock
- Mapeamento de periféricos específicos do board

---

### 📚 `docs/`
**Propósito**: Documentação técnica e arquitetural do projeto.

```
docs/
├── architecture.md               # Diagrama de arquitetura do sistema
└── estrutura_repositorio.md      # Este documento
```

**Arquivos**:
- `architecture.md`: Diagrama Mermaid da arquitetura do sistema, mostrando as camadas USB, protocolos, HSM e hardware
- `estrutura_repositorio.md`: Documentação completa da estrutura do repositório

---

### 🛠️ `host_tools/`
**Propósito**: Ferramentas de linha de comando, GUI e SDK Python para desenvolvimento e uso do OpenToken no host (PC).

```
host_tools/
├── opentoken_authenticator.py    # Autenticador principal
├── opentoken_cli.py              # Interface de linha de comando (CLI)
├── opentoken_gui.py              # Interface gráfica básica
├── opentoken_gui_pro.py          # Interface gráfica avançada (Pro)
└── opentoken_sdk/                # SDK Python oficial
    ├── __init__.py
    ├── opentoken.py              # Implementação principal do SDK
    └── __pycache__/              # Cache Python (gerado automaticamente)
```

**Funcionalidades**:
- **CLI**: Gerenciamento via terminal (adicionar credenciais, gerar TOTP, etc.)
- **GUI**: Interface gráfica para usuários finais
- **SDK**: Biblioteca Python reutilizável para integração em outros projetos

---

### 📋 `include/`
**Propósito**: Arquivos de cabeçalho (headers) C/C++ que definem interfaces e estruturas do firmware.

```
include/
├── cbor_utils.h                  # Utilitários para codificação/decodificação CBOR
├── ccid_device.h                 # Interface do dispositivo CCID USB
├── ccid_engine.h                 # Motor de processamento CCID/APDU
├── ctap2_engine.h                # Motor FIDO2/CTAP2
├── error_handling.h              # Sistema de tratamento de erros
├── error_handling_test.h         # Testes de tratamento de erros
├── hsm_layer.h                   # Camada HSM (Hardware Security Module)
├── led_status.h                  # Controle do LED RGB WS2812
├── libopentoken.h                # Cabeçalho principal da biblioteca
├── mbedtls_config.h              # Configuração do mbedTLS
├── oath_applet.h                 # Applet OATH (TOTP/HOTP)
├── openpgp_applet.h              # Applet OpenPGP Card
├── opentoken.h                   # Cabeçalho principal do firmware
├── storage.h                     # Interface de armazenamento seguro
└── tusb_config.h                 # Configuração do TinyUSB
```

**Organização por Módulo**:
- **Protocolos**: `ctap2_engine.h`, `ccid_engine.h`, `oath_applet.h`, `openpgp_applet.h`
- **Hardware**: `ccid_device.h`, `led_status.h`, `storage.h`
- **Criptografia**: `hsm_layer.h`, `mbedtls_config.h`
- **Utilitários**: `cbor_utils.h`, `error_handling.h`

---

### 📚 `libraries/`
**Propósito**: Bibliotecas cliente oficiais para diferentes linguagens de programação, permitindo integração do OpenToken em aplicações externas.

```
libraries/
├── libopentoken/                 # Biblioteca C (core)
│   └── libopentoken.c
├── opentoken-dart/               # SDK Dart/Flutter
│   ├── lib/
│   │   ├── opentoken_dart.dart
│   │   ├── opentoken_service.dart
│   │   └── transports/
│   │       ├── transport_nfc.dart
│   │       └── transport_usb.dart
│   ├── LICENSE
│   ├── pubspec.yaml              # Dependências Dart
│   ├── pubspec.lock
│   └── README.md
└── python-opentoken/             # SDK Python (PyPI)
    ├── src/
    │   └── opentoken/
    │       ├── __init__.py
    │       └── core.py
    ├── LICENSE
    ├── pyproject.toml            # Configuração do pacote Python
    └── README.md
```

**Bibliotecas Disponíveis**:

1. **libopentoken** (C)
   - Biblioteca core em C para integração em sistemas embarcados ou aplicações C/C++

2. **opentoken-dart** (Dart/Flutter)
   - SDK oficial para desenvolvimento de aplicações Flutter (mobile e desktop)
   - Suporta transportes USB e NFC
   - Usado pelo `opentoken_app/`

3. **python-opentoken** (Python)
   - SDK Python oficial, disponível via PyPI
   - Dependências: `pyusb`, `pyscard`, `pyperclip`
   - Usado pelas ferramentas em `host_tools/`

---

### 📱 `opentoken_app/`
**Propósito**: Aplicativo Flutter oficial (OpenToken NATIVO) para Desktop e Mobile.

```
opentoken_app/
├── android/                      # Configuração Android
│   └── app/src/main/java/...
├── ios/                          # Configuração iOS
│   ├── Flutter/...
│   └── Runner/...
├── windows/                      # Configuração Windows
│   ├── CMakeLists.txt
│   └── runner/...
├── lib/                          # Código-fonte Dart
│   ├── main.dart                 # Ponto de entrada
│   ├── theme.dart                # Tema da aplicação
│   └── widgets/                  # Componentes da UI
│       ├── add_credential_view.dart
│       ├── credentials_view.dart
│       ├── crypto_management_view.dart
│       ├── device_status_view.dart
│       ├── device_status.dart
│       ├── premium_card.dart
│       └── settings_view.dart
├── test/                         # Testes unitários
│   └── widget_test.dart
├── android_nfc_config.xml.txt   # Configuração NFC Android
├── ios_nfc_config.plist.txt      # Configuração NFC iOS
├── analysis_options.yaml         # Configuração do analisador Dart
├── pubspec.yaml                  # Dependências Flutter
├── pubspec.lock
├── LICENSE
└── README.md
```

**Funcionalidades do App**:
- Gerenciamento de credenciais FIDO2
- Geração de códigos TOTP/HOTP
- Gerenciamento de chaves OpenPGP
- Status do dispositivo
- Configurações e personalização

---

### 💻 `src/`
**Propósito**: Código-fonte principal do firmware (implementação em C).

```
src/
├── main.c                        # Ponto de entrada do firmware
├── usb_descriptors.c             # Descritores USB (HID, CCID, Vendor)
├── storage.c                     # Implementação de armazenamento seguro
├── cbor_utils.c                  # Utilitários CBOR
├── hsm_layer.c                   # Camada HSM (operações criptográficas)
├── ctap2_engine.c                # Motor FIDO2/CTAP2
├── ccid_engine.c                 # Motor CCID/APDU
├── oath_applet.c                 # Applet OATH (TOTP/HOTP)
├── openpgp_applet.c              # Applet OpenPGP Card
├── error_handling.c              # Tratamento de erros
├── ccid_device.c                 # Driver CCID USB
├── webusb_handler.c              # Handler WebUSB para gerenciamento
├── led_status.c                  # Controle do LED RGB
├── otp_keyboard.c                # Emulação de teclado para OTP
└── ws2812.pio                    # Programa PIO para LED WS2812
```

**Módulos Principais**:

1. **Core**:
   - `main.c`: Inicialização, loop principal, gerenciamento de eventos
   - `usb_descriptors.c`: Configuração USB composite (HID, CCID, Vendor)

2. **Protocolos**:
   - `ctap2_engine.c`: Implementação FIDO2/CTAP2 (WebAuthn, U2F)
   - `ccid_engine.c`: Processamento de comandos APDU CCID
   - `oath_applet.c`: Applet OATH (RFC 6238/4226)
   - `openpgp_applet.c`: Applet OpenPGP Card

3. **Segurança**:
   - `hsm_layer.c`: Abstração criptográfica (RSA, ECC, Ed25519)
   - `storage.c`: Armazenamento seguro em Flash criptografado

4. **Hardware**:
   - `ccid_device.c`: Driver USB CCID customizado
   - `led_status.c`: Feedback visual via LED RGB
   - `ws2812.pio`: Programa PIO para controle do LED
   - `otp_keyboard.c`: Emulação de teclado HID para inserção de OTP

5. **Utilitários**:
   - `cbor_utils.c`: Codificação/decodificação CBOR
   - `error_handling.c`: Sistema robusto de tratamento de erros
   - `webusb_handler.c`: Interface WebUSB para gerenciamento remoto

---

## 🔨 Arquivos de Build e Configuração

### `CMakeLists.txt`
Arquivo principal de configuração CMake que:
- Configura o projeto para RP2350/Pico SDK
- Define os arquivos fonte a compilar
- Configura bibliotecas (TinyUSB, mbedTLS, Pico SDK)
- Define flags de compilação USB (CCID, HID, Vendor)
- Gera saídas UF2 para upload fácil

### `pico_sdk_init.cmake`
Script de inicialização do Raspberry Pi Pico SDK, configurado para buscar automaticamente o SDK via Git.

---

## 📦 Dependências Principais

### Firmware (Embedded)
- **Raspberry Pi Pico SDK**: SDK oficial para RP2040/RP2350
- **TinyUSB**: Stack USB device para composite device
- **mbedTLS 3.x**: Biblioteca criptográfica (RSA, ECC, Ed25519)

### Host Tools (Python)
- `pyusb`: Comunicação USB
- `pyscard`: Interface com smart cards (CCID)
- `pyperclip`: Manipulação de clipboard

### App Flutter
- SDK Dart/Flutter 3.0+
- `opentoken-dart`: SDK Dart oficial (local)

---

## 🔄 Fluxo de Build

### Firmware
```bash
cmake -B build -G "MinGW Makefiles" -DPICO_SDK_FETCH_FROM_GIT=ON
cmake --build build
```

**Saídas**:
- `build/opentoken_firmware.uf2`: Arquivo para upload via bootloader
- `build/opentoken_firmware.elf`: Executável ELF para debug

### Bibliotecas
- **Python**: `pip install -e libraries/python-opentoken/`
- **Dart**: Adicionado como dependência local no `pubspec.yaml` do app

---

## 🎯 Protocolos Suportados

| Protocolo | Interface USB | Implementação |
|-----------|---------------|---------------|
| **FIDO2/CTAP2** | HID | `ctap2_engine.c` |
| **OATH TOTP/HOTP** | CCID | `oath_applet.c` |
| **OpenPGP Card** | CCID | `openpgp_applet.c` |
| **WebUSB** | Vendor | `webusb_handler.c` |
| **OTP Keyboard** | HID Keyboard | `otp_keyboard.c` |

---

## 🔐 Camadas de Segurança

1. **Secure Storage** (`storage.c`): Flash isolado e criptografado
2. **HSM Layer** (`hsm_layer.c`): Abstração criptográfica unificada
3. **Protocol Engines**: Motores de estado independentes sem vazamento de contexto
4. **Hardware Root of Trust**: Chaves criptográficas protegidas por hardware

---

## 📝 Convenções de Código

- **Código C**: Segue padrões do Pico SDK
- **Python**: PEP 8
- **Dart**: Effective Dart guidelines
- **Documentação**: Markdown com diagramas Mermaid

---

## 🤝 Contribuindo

Veja `CONTRIBUTING.md` para diretrizes de contribuição. O projeto segue uma política "100% NATIVO" - sem dependências de marcas proprietárias.

---

## 📄 Licença

MIT License - Veja `LICENSE` para detalhes.

---

**Última atualização**: 2024
**Versão do Documento**: 1.0

