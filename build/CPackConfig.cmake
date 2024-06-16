# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/Users/alealfaro/pico/Badger2040-MQTT;/Users/alealfaro/pico/Badger2040-MQTT/build")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/opt/homebrew/Cellar/cmake/3.25.2/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "BadgerMQTT built using CMake")
set(CPACK_GENERATOR "ZIP;TGZ")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY "OFF")
set(CPACK_INSTALL_CMAKE_PROJECTS "/Users/alealfaro/pico/Badger2040-MQTT/build;BadgerMQTT;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/Users/alealfaro/pico/pico-sdk/cmake;/Users/alealfaro/pico/pimoroni-pico")
set(CPACK_NSIS_DISPLAY_NAME "BadgerMQTT 0.1.1")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "BadgerMQTT 0.1.1")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OBJCOPY_EXECUTABLE "/opt/homebrew/bin/arm-none-eabi-objcopy")
set(CPACK_OBJDUMP_EXECUTABLE "/opt/homebrew/bin/arm-none-eabi-objdump")
set(CPACK_OUTPUT_CONFIG_FILE "/Users/alealfaro/pico/Badger2040-MQTT/build/CPackConfig.cmake")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/opt/homebrew/Cellar/cmake/3.25.2/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BadgerMQTT built using CMake")
set(CPACK_PACKAGE_FILE_NAME "BadgerMQTT-0.1.1-PICO")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "BadgerMQTT 0.1.1")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "BadgerMQTT 0.1.1")
set(CPACK_PACKAGE_NAME "BadgerMQTT")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Humanity")
set(CPACK_PACKAGE_VERSION "0.1.1")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_READELF_EXECUTABLE "/opt/homebrew/bin/arm-none-eabi-readelf")
set(CPACK_RESOURCE_FILE_LICENSE "/opt/homebrew/Cellar/cmake/3.25.2/share/cmake/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "/opt/homebrew/Cellar/cmake/3.25.2/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/opt/homebrew/Cellar/cmake/3.25.2/share/cmake/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_7Z "ON")
set(CPACK_SOURCE_GENERATOR "7Z;ZIP")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/Users/alealfaro/pico/Badger2040-MQTT/build/CPackSourceConfig.cmake")
set(CPACK_SOURCE_ZIP "ON")
set(CPACK_SYSTEM_NAME "PICO")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "PICO")
set(CPACK_WIX_SIZEOF_VOID_P "4")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/Users/alealfaro/pico/Badger2040-MQTT/build/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
