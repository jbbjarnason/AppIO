project(appio_lua)

find_package(Lua REQUIRED)

add_library(appio_lua SHARED LuaBinding.cpp)
target_link_libraries(appio_lua
        ${LUA_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${ZeroMQ_LIBRARY}
        ${Boost_LIBRARIES})

include_directories(
        ${LUA_INCLUDE_DIR})