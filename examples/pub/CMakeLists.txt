project(pub)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT}
                        ${ZeroMQ_LIBRARY}
                        ${Boost_LIBRARIES})