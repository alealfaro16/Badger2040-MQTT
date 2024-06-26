# Add library cpp files

if (NOT DEFINED TINY_JSON_DIR)
    set(TINY_JSON_DIR "~/c-libs/tiny-json")
endif()

add_library(tiny_json INTERFACE)
target_sources(tiny_json INTERFACE
    ${TINY_JSON_DIR}/tiny-json.c
)

# Add include directory
target_include_directories(tiny_json INTERFACE 
   ${TINY_JSON_DIR}/
)

# Add the standard library to the build
target_link_libraries(tiny_json INTERFACE pico_stdlib)