cmake_minimum_required(VERSION 3.13)

# Configuração para o SDK do Raspberry Pi Pico (RP2040/RP2350)
set(PICO_SDK_FETCH_FROM_GIT ON)
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
    src/yubikey_mgmt.c
    src/error_handling.c
    src/ccid_device.c
)

# Adiciona o diretório de includes
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
)

# TinyUSB Configuration for Composite Device
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_flash
    hardware_sync
    pico_mbedtls # Cryptographic operations
)

# Enable stdio over UART (not USB to avoid conflicts with our composite device)
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# USB Composite Device Configuration
target_compile_definitions(${PROJECT_NAME} PRIVATE
    # Enable required USB classes
    CFG_TUD_HID=1
    CFG_TUD_CCID=1
    
    # Disable unused USB classes
    CFG_TUD_CDC=0
    CFG_TUD_MSC=0
    CFG_TUD_MIDI=0
    CFG_TUD_VENDOR=0
    CFG_TUD_USBTMC=0
    CFG_TUD_DFU_RUNTIME=0
    CFG_TUD_AUDIO=0
    CFG_TUD_VIDEO=0
    CFG_TUD_NET=0
    CFG_TUD_BTH=0
    
    # Endpoint configuration
    CFG_TUD_ENDPOINT0_SIZE=64
    
    # HID Configuration
    CFG_TUD_HID_EP_BUFSIZE=64
    
    # CCID Configuration  
    CFG_TUD_CCID_EP_BUFSIZE=64
    CFG_TUD_CCID_MAX_SLOTS=1
)

# Adiciona o arquivo UF2 para upload fácil
pico_add_extra_outputs(${PROJECT_NAME})
