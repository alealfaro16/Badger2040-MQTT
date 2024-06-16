# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/alealfaro/pico/pico-sdk/tools/pioasm"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pioasm"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/tmp"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src"
  "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/alealfaro/pico/Badger2040-MQTT/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
