cmake_minimum_required(VERSION 3.1)
project(MoeLuaWrapper)

# 第三方模块
if(NOT TARGET liblua-static)
    add_subdirectory(3rd/LuaJIT)
endif()

# 编译选项
set(CMAKE_CXX_STANDARD 11)
if(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0600 -D_GNU_SOURCE -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-function")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-function")
endif()

add_library(MoeLuaWrapper STATIC src/Stub.cpp)
target_link_libraries(MoeLuaWrapper liblua-static)
target_include_directories(MoeLuaWrapper PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
