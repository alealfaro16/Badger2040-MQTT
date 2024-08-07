target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/MQTTAgent.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MQTTAgentObserver.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MQTTInterface.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MQTTRouter.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MQTTRouterBadger.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/MQTTTopicHelper.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
