target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/TCPTransport.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/WifiHelper.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/TLSTransBlock.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/Transport.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
