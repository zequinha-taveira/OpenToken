cmake_minimum_required(VERSION 3.13)

# Configuração para o SDK do Raspberry Pi Pico (RP2040/RP2350)
include(pico_sdk_init.cmake)

project(opentoken_firmware C CXX)

# Inicializa o SDK
pico_sdk_init()

# Adiciona o diretório de código fonte
add_executable(${PROJECT_NAME}
    src/main.c
    src/usb_descriptors.c
    src/storage.c
    src/cbor_utils.c
    src/hsm_layer.c
    src/ctap2_engine.c
    src/ccid_engine.c
    src/oath_applet.c
    src/openpgp_applet.c
)

# Adiciona o diretório de includes
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
)

# Configurações para o TinyUSB
# Assume que o TinyUSB será adicionado como um submodule ou via pico_add_extra_dependency
# Para simplificar, vamos apenas adicionar as bibliotecas necessárias para USB
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_cyw43_driver # Se for usar RP2350 com Wi-Fi
    tinyusb_device
    hardware_flash
    pico_mbedtls # Crypto
)

# Configurações de compilação
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Define as interfaces USB (HID e CCID)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CFG_TUD_HID=1
    CFG_TUD_MSC=0
    CFG_TUD_CDC=0
    CFG_TUD_MIDI=0
    CFG_TUD_VENDOR=0
    CFG_TUD_USBTMC=0
    CFG_TUD_DFU_RUNTIME=0
    CFG_TUD_CDC_ACM_LINE_CODING_SUPPORT=0
    CFG_TUD_CCID=1 # Habilita a interface CCID para OATH/OpenPGP
)

# Adiciona o arquivo UF2 para upload fácil
pico_add_extra_outputs(${PROJECT_NAME})
