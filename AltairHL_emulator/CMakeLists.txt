#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (AltairHL_emulator C)

# SELECT DEVELOPER BOARD ##########################################################################################
#
# set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1 ")   
# set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2 ")  
set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or rdb)")
set(SEEED_STUDIO_RDB_ETHERNET TRUE "Seeed Studio Azure Sphere MT3620 Development Kit with ethernet shield")
# set(SEEED_STUDIO_MDB TRUE "Seeed Studio Azure Sphere Mini Developer Board")
#
###################################################################################################################

# SELECT FRONT PANEL CONFIG #######################################################################################
#
set(ALTAIR_FRONT_PANEL_NONE TRUE "Altair on Azure Sphere with no panel.")
# set(ALTAIR_FRONT_PANEL_RETRO_CLICK TRUE "Avnet with the MikroE 8800 Retro Click")
# set(ALTAIR_FRONT_PANEL_CLICK TRUE "Avnet with the MikroE 8x8R Click")
# set(ALTAIR_FRONT_PANEL_KIT TRUE "Altair front panel board")
#
###################################################################################################################

# ENABLE SD CARD ##################################################################################################
#
# The MikroE microSD Click works with the Avnet Azure Sphere Starter Kit Rev 1 and Rev 2 when in Socket 1.
# The MikroE microSD Click works with the Mikroe 8800 Retro Click in Socket 2
# Uncomment the "set(MICRO_SD_CLICK" line below to enable the MikroE microSD Click
#
# set(MICRO_SD_CLICK TRUE "MikroE microSD Click for CP/M read/write to SD Card")
#
###################################################################################################################

###################################################################################################################
#
# set(ALTAIR_AS_SERVICE TRUE "Enable Altair as a service real-time in-memory differencing disk")
###################################################################################################################

###################################################################################################################
# IoT Hub Polling time overide
add_compile_definitions(IOT_HUB_POLL_TIME_NANOSECONDS=500000000)
###################################################################################################################

if (ALTAIR_AS_SERVICE AND MICRO_SD_CLICK)
  message(FATAL_ERROR, "YOU CANNOT HAVE ALTAIR AS A SERVICE AND SD CARD SELECTED AT THE SAME TIME")
endif(ALTAIR_AS_SERVICE AND MICRO_SD_CLICK)

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

if (EXISTS "${PARENT_DIR}/Tools/cmake/azsphere_config.cmake")
    include(../Tools/cmake/azsphere_config.cmake)
    auto_generate_azsphere_config()
else()

    # For information on setting tools revision and target api set see 
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.07")
    azsphere_configure_api(TARGET_API_SET "10")

endif()

add_subdirectory("AzureSphereDevX" out)

add_compile_definitions(AZURE_SPHERE)

if (ALTAIR_AS_SERVICE)
    add_compile_definitions(ALTAIR_SERVICE)
endif(ALTAIR_AS_SERVICE)

if(AVNET OR AVNET_REV_2)
    add_subdirectory("Drivers/AVNET/HL" drivers)
endif(AVNET OR AVNET_REV_2)

if (MICRO_SD_CLICK)
    add_compile_definitions(SD_CARD_ENABLED)
endif (MICRO_SD_CLICK)

if (ALTAIR_FRONT_PANEL_KIT)

    add_compile_definitions(ALTAIR_FRONT_PANEL_KIT)

    set(Oem
        "FrontPanels/front_panel_kit.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_KIT)

if (ALTAIR_FRONT_PANEL_CLICK)

    add_compile_definitions(ALTAIR_FRONT_PANEL_CLICK)

    set(Oem
        "FrontPanels/front_panel_click.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_CLICK)

if (ALTAIR_FRONT_PANEL_RETRO_CLICK)

    add_compile_definitions(ALTAIR_FRONT_PANEL_RETRO_CLICK)

    set(Oem
        "FrontPanels/front_panel_retro_click.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_RETRO_CLICK)

if (ALTAIR_FRONT_PANEL_NONE)

    add_compile_definitions(ALTAIR_FRONT_PANEL_NONE)

    set(Oem
        "FrontPanels/front_panel_none.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_NONE)


set(Source
    "altair_config.c"
    "Altair8800/88dcdd.c"
    "Altair8800/intel8080.c"
    "Altair8800/memory.c"
    "cpu_monitor.c"
    "env_open_weather_map.c"
    "environment.c"
    "io_ports.c"
    "iotc_manager.c"
    "location_from_ip.c"
    "main.c"
    "onboard_sensors.c"
    "utils.c"
    "web_socket_server.c"
)
source_group("Source" FILES ${Source})


set(wsServer
    wsServer/src/ws.c
    wsServer/src/base64/base64.c
    wsServer/src/sha1/sha1.c # sha1.c is pulled in from the Azure IoT SDK C
    wsServer/src/handshake/handshake.c
    wsServer/src/utf8/utf8.c
)
source_group("wsServer" FILES ${wsServer})

set(ALL_FILES
    ${Source}
    ${Oem}
    ${wsServer}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c azure_sphere_devx wolfssl tlsutils curl)
target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )
target_include_directories(${PROJECT_NAME} PUBLIC ../IntercoreContract)
target_include_directories(${PROJECT_NAME} PRIVATE wsServer/include)
include_directories(${CMAKE_SOURCE_DIR} FrontPanels)
include_directories(${CMAKE_SOURCE_DIR} Altair8800)

if(AVNET OR AVNET_REV_2)
    target_link_libraries (${PROJECT_NAME} avnet_drivers)
    target_include_directories(${PROJECT_NAME} PUBLIC Drivers/AVNET/HL)
endif(AVNET OR AVNET_REV_2)


target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)

set_source_files_properties(Altair8800/intel8080.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/intel8080.h PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/memory.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(comms_manager.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(FrontPanels/front_panel_virtual.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/ws.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/web_socket_server.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/base64/base64.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/handshake/handshake.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/sha1/sha1.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wsServer/src/utf8/utf8.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)


if(ALTAIR_FRONT_PANEL_CLICK)

add_subdirectory("Drivers/CLICK_8x8_LED_MAX7219" max7219_driver)
include_directories(${CMAKE_SOURCE_DIR} Drivers/CLICK_8x8_LED_MAX7219)
target_link_libraries(${PROJECT_NAME} max7219_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_8x8_LED_MAX7219 )

add_subdirectory("Drivers/CLICK_4x4_KEY_74HC165" 74HC165_driver)
include_directories(${CMAKE_SOURCE_DIR} Drivers/CLICK_4x4_KEY_74HC165)
target_link_libraries(${PROJECT_NAME} 74HC165_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_4x4_KEY_74HC165 )

endif(ALTAIR_FRONT_PANEL_CLICK)

if(ALTAIR_FRONT_PANEL_RETRO_CLICK)

add_subdirectory("Drivers/CLICK_8800_RETRO_AS1115" as1115_driver)
include_directories(${PROJECT_NAME} Drivers/CLICK_8800_RETRO_AS1115)
target_link_libraries(${PROJECT_NAME} as1115_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_8800_RETRO_AS1115 )

endif(ALTAIR_FRONT_PANEL_RETRO_CLICK)

if(AVNET)    
    add_compile_definitions(OEM_AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "altair.json")
endif(AVNET)

if(AVNET_REV_2)
    add_compile_definitions(OEM_AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "altair.json")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)   
    add_compile_definitions(OEM_SEEED_STUDIO)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "altair.json")
endif()

if (SEEED_STUDIO_RDB_ETHERNET)   
    add_compile_definitions(OEM_SEEED_STUDIO)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/mt3620_rdb_ethernet" TARGET_DEFINITION "altair.json")
endif()

if (SEEED_STUDIO_MDB)   
    add_compile_definitions(OEM_SEEED_STUDIO)    
    add_compile_definitions(SEEED_STUDIO_MDB)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/seeed_mt3620_mdb" TARGET_DEFINITION "altair.json")
endif()


# If MICRO_SD_CLICK enabled then don't deploy the "Disks/cpm63k.dsk" disk image as CP/M will be booted from microSD Card. Reduces deploymemt time.
if (MICRO_SD_CLICK)
    azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES  "BasicSamples/STARTREK.BAS" "BasicSamples/TICTACTOE.BAS" "BasicSamples/SIMPLE.BAS" "BasicSamples/LOOPY.BAS" "BasicSamples/WEATHER.BAS" "BasicSamples/IOT.BAS" "Disks/88dskrom.bin" "Disks/altair_basic.bin")
else (MICRO_SD_CLICK)
    azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES  "BasicSamples/STARTREK.BAS" "BasicSamples/TICTACTOE.BAS" "BasicSamples/SIMPLE.BAS" "BasicSamples/LOOPY.BAS" "BasicSamples/WEATHER.BAS" "BasicSamples/IOT.BAS" "Disks/88dskrom.bin" "Disks/azsphere_cpm63k.dsk"  "Disks/altair_basic.bin")
endif (MICRO_SD_CLICK)
