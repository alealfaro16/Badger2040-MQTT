target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/TimeView.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MessageView.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/ReminderView.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/EventView.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
