target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/GPIOObserver.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/SwitchObserver.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/GPIOInputMgr.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/SwitchMgr.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
