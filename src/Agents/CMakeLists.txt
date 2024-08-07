target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/Agent.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/BadgerAgent.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
