# Arquitetura do Sistema OpenToken

O OpenToken comunica-se através de protocolos padrão para garantir a máxima compatibilidade sem drivers proprietários. Este diagrama ilustra como o firmware OpenToken organiza as interfaces USB e processa os diferentes protocolos.

```mermaid
graph TD
    %% Estilos
    classDef host fill:#e1f5fe,stroke:#01579b,stroke-width:2px;
    classDef usb fill:#fff3e0,stroke:#e65100,stroke-width:2px;
    classDef proto fill:#f3e5f5,stroke:#4a148c,stroke-width:2px;
    classDef hsm fill:#e8f5e9,stroke:#1b5e20,stroke-width:2px;
    classDef hw fill:#fafafa,stroke:#212121,stroke-width:2px;

    subgraph Host_Tools ["FERRAMENTAS HOST"]
        direction TB
        Auth[OpenToken NATIVO Auth]:::host
        Browser[Navegador]:::host
        GPG[GnuPG/OpenSC]:::host
        TextEditor[Editor de Texto]:::host
    end

    %% Conexões com Interfaces USB
    Auth -->|CCID/APDU| USB_CCID
    GPG -->|CCID/APDU| USB_CCID
    Browser -->|HID/CTAP2| USB_FIDO
    Browser -->|WebUSB| USB_Vendor
    TextEditor -->|HID/Keyboard| USB_KBD

    subgraph Device ["USB COMPOSITE DEVICE (TinyUSB)"]
        direction TB
        USB_CCID[Interface CCID<br/>(OATH + OpenPGP)]:::usb
        USB_FIDO[Interface HID FIDO2]:::usb
        USB_Vendor[Interface WebUSB<br/>(Gerenciamento)]:::usb
        USB_KBD[Interface HID Teclado<br/>(OTP)]:::usb
    end

    %% Conexões com Camada de Protocolo
    USB_CCID --> APDU_Dispatch
    USB_FIDO --> CTAP_Engine
    USB_Vendor --> Web_Handler
    USB_KBD --> OTP_Logic

    subgraph Protocol_Layer ["CAMADA DE PROTOCOLOS"]
        direction TB
        APDU_Dispatch{APDU Dispatcher}:::proto
        OATH_Engine[OATH Engine<br/>RFC 6238/4226]:::proto
        PGP_Engine[OpenPGP Card<br/>Engine]:::proto
        CTAP_Engine[CTAP2 Engine]:::proto
        Web_Handler[WebUSB Handler]:::proto
        OTP_Logic[OTP Keyboard Logic]:::proto
    end

    %% Roteamento APDU
    APDU_Dispatch -->|Select OATH| OATH_Engine
    APDU_Dispatch -->|Select OpenPGP| PGP_Engine

    %% Conexões com HSM
    OATH_Engine --> HSM
    PGP_Engine --> HSM
    CTAP_Engine --> HSM
    OTP_Logic --> HSM
    Web_Handler --> HSM

    subgraph HSM_Layer ["HSM LAYER"]
        HSM[• Armazenamento Seguro (Flash Criptografado)<br/>• Gerenciamento de Chaves (ECDSA, RSA, Ed25519)<br/>• Motor Criptográfico (mbedTLS)<br/>• PIN/Contadores]:::hsm
    end
    
    HSM --> Hardware

    subgraph Hardware ["HARDWARE: RP2350"]
        Hardware_Node[• Dual-core ARM Cortex-M33 @ 150MHz<br/>• 16MB Flash (W25Q128)<br/>• USB Type-A integrado<br/>• LED RGB (GP22)<br/>• Botão Físico]:::hw
    end
```
