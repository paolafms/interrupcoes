# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(interrupcoes C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(interrupcoes interrupcoes.c)

pico_set_program_name(interrupcoes "interrupcoes")
pico_set_program_version(interrupcoes "0.1")

# Habilitar saída de depuração via USB
pico_enable_stdio_uart(interrupcoes 0)
pico_enable_stdio_usb(interrupcoes 1)

# Adicionar bibliotecas necessárias
target_link_libraries(interrupcoes
    pico_stdlib
    hardware_gpio
    hardware_irq
    hardware_timer
    hardware_pio
)

# Adicionar o código do PIO para os LEDs WS2812
pico_generate_pio_header(interrupcoes ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Adicionar diretórios de include
target_include_directories(interrupcoes PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Adicionar saídas extras (como arquivos .uf2)
pico_add_extra_outputs(interrupcoes)