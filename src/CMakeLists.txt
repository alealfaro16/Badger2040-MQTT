add_executable(${NAME}
        main.cpp
        ble_server.cpp
        WifiHelper.cpp
        TCPTransport.cpp
        MQTTAgent.cpp
        MQTTInterface.cpp
        MQTTAgentObserver.cpp
        MQTTTopicHelper.cpp
        Agent.cpp
        GPIOInputMgr.cpp
        GPIOObserver.cpp
        LEDAgent.cpp
        SwitchMgr.cpp
        SwitchObserver.cpp
        MQTTRouter.cpp
        MQTTRouterLED.cpp
        BadgerAgent.cpp
        MQTTRouterBadger.cpp
        )


include(libraries/badger2040/badger2040)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(${NAME} 
	 pico_stdlib
     FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
     FREERTOS_PORT
     LWIP_PORT
     pico_cyw43_arch_lwip_sys_freertos
     pico_btstack_ble      # BLE stack
     pico_btstack_cyw43
     coreMQTT
     coreMQTTAgent
     tiny_json
     badger2040
     hardware_rtc
     hardware_adc
     hardware_gpio
     hardware_pwm
)
# run the compile_gatt compiler to create a GATT file from the BTstack GATT file:
pico_btstack_make_gatt_header(${NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
     )

add_subdirectory(Views)
add_subdirectory(NVS)

if (NOT DEFINED ENV{WIFI_SSID})
	message(FATAL_ERROR "WIFI_SSID Environment variable not defined")
endif()
if (NOT DEFINED ENV{WIFI_PASSWORD})
	message(FATAL_ERROR "WIFI_PASSWORD Environment variable not defined")
endif()
if (NOT DEFINED ENV{MQTT_CLIENT})
	message(FATAL_ERROR "MQTT_CLIENT Environment variable not defined")
endif()
if (NOT DEFINED ENV{MQTT_USER})
	message(FATAL_ERROR "MQTT_USER Environment variable not defined")
endif()
if (NOT DEFINED ENV{MQTT_PASSWD})
	message(FATAL_ERROR "MQTT_PASSWD Environment variable not defined")
endif()
if (NOT DEFINED ENV{MQTT_HOST})
	message(FATAL_ERROR "MQTT_HOST Environment variable not defined")
endif()
if (NOT DEFINED ENV{MQTT_PORT})
	message(FATAL_ERROR "MQTT_PORT Environment variable not defined")
endif()


target_compile_definitions(${NAME} PRIVATE
    WIFI_SSID=\"$ENV{WIFI_SSID}\"
    WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
    CYW43_HOST_NAME="PicoW2"
    MQTT_CLIENT=\"$ENV{MQTT_CLIENT}\"
    MQTT_USER=\"$ENV{MQTT_USER}\"
    MQTT_PASSWD=\"$ENV{MQTT_PASSWD}\"
    MQTT_HOST=\"$ENV{MQTT_HOST}\"
    MQTT_PORT=$ENV{MQTT_PORT}
    WIFI_ENABLED=1
    BLE_ENABLED=0
    CYW43_ENABLE_BLUETOOTH=BLE_ENABLED  # otherwise cyw43_arch_init() will not initialize BLE
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# enable usb output, disable uart output
pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)
