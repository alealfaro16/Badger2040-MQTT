target_sources(${NAME} PRIVATE  ${CMAKE_CURRENT_LIST_DIR}/Request.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/RequestObserver.cpp
                                ${CMAKE_CURRENT_LIST_DIR}/WeatherServiceRequest.cpp
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
