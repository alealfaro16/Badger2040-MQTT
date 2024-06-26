cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME BadgerMQTT) # <-- Name your project/executable here!

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
pico_sdk_init()

set(TWIN_THING_PICO_CONFIG_PATH ${CMAKE_CURRENT_LIST_DIR}/port/twinThing)
set(COREMQTT_PORT_PATH      ${CMAKE_CURRENT_LIST_DIR}/port/CoreMQTT)
set(COREMQTT_AGENT_PORT_PATH      ${CMAKE_CURRENT_LIST_DIR}/port/CoreMQTT-Agent)


include(lwip_import.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#SET(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../lib/FreeRTOS-Kernel" CACHE STRING "Course Common Lib")
SET(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/port/FreeRTOS-Kernel" CACHE STRING "Local Config")
include_directories("${FREERTOS_CONFIG_FILE_DIRECTORY}") 
include(FreeRTOS_Kernel_import.cmake)

set(CORE_MQTT_DIR "$ENV{CORE_MQTT_PATH}" CACHE STRING "Course Common Lib")
include("FreeRTOS-coreMQTT.cmake")

set(CORE_MQTT_AGENT_DIR "$ENV{CORE_MQTT_AGENT_PATH}" CACHE STRING "Course Common Lib")
include("FreeRTOS-coreMQTT-Agent.cmake")

set(TINY_JSON_DIR "${CMAKE_CURRENT_LIST_DIR}/../../c-libs/tiny-json" CACHE STRING "Course Common Lib")
include("tiny-json.cmake")

include(pimoroni_pico_import.cmake)

include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/port/twinThing"
)

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
