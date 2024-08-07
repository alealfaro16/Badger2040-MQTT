cmake_minimum_required(VERSION 3.24) #was 3.12
SET(CMAKE_COLOR_DIAGNOSTICS ON)
# Change your executable name to something creative!
set(NAME BadgerMQTT) # <-- Name your project/executable here!

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

set(PICO_EXTRAS_PATH "${CMAKE_CURRENT_LIST_DIR}/../../pico/pico-extras" CACHE STRING "Course Common Lib")
include(pico_extras_import.cmake)
# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)
pico_sdk_init()

set(TWIN_THING_PICO_CONFIG_PATH ${CMAKE_CURRENT_LIST_DIR}/port/twinThing)
set(COREMQTT_PORT_PATH      ${CMAKE_CURRENT_LIST_DIR}/port/CoreMQTT)
set(COREMQTT_AGENT_PORT_PATH      ${CMAKE_CURRENT_LIST_DIR}/port/CoreMQTT-Agent)


include(lwip_import.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/FreeRTOS-Kernel" CACHE STRING "Course Common Lib")
SET(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/port/FreeRTOS-Kernel" CACHE STRING "Local Config")
include_directories("${FREERTOS_CONFIG_FILE_DIRECTORY}") 
include(FreeRTOS_Kernel_import.cmake)

set(CORE_MQTT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/coreMQTT" CACHE STRING "Course Common Lib")
include("FreeRTOS-coreMQTT.cmake")

set(CORE_MQTT_AGENT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/coreMQTT-Agent" CACHE STRING "Course Common Lib")
include("FreeRTOS-coreMQTT-Agent.cmake")

set(COREHTTP_PATH "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/coreHTTP" CACHE STRING "Course Common Lib")
set(COREHTTP_PORT "${CMAKE_CURRENT_LIST_DIR}/port/CoreHTTP/" CACHE STRING "Course Common Lib")
include(coreHTTP.cmake)

set(WOLFSSL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/wolfssl" CACHE STRING "Course Common Lib")
include("wolfssl_import.cmake")

SET(URILIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/uri-library" CACHE STRING "Common Lib")
include(urilib.cmake)

SET(JSON_MAKER_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/json-maker" CACHE STRING "Common Lib")
include(json-maker.cmake)

set(TINY_JSON_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/tiny-json" CACHE STRING "Course Common Lib")
include("tiny-json.cmake")

set(SPNG_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/libspng" CACHE STRING "Common Lib")
set(MINIZ_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/miniz" CACHE STRING "Common Lib")
include(spng_import.cmake)

include(pimoroni_pico_import.cmake)


add_subdirectory(src)

#Set up files for the release packages
install(CODE "execute_process(COMMAND $ENV{HOME}/bin/picoDeploy.sh ${CMAKE_CURRENT_BINARY_DIR}/src/${NAME}.elf)")

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/src/${NAME}.uf2
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
