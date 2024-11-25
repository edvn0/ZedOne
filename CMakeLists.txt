cmake_minimum_required(VERSION 3.27)

project(ZedOne VERSION 1.0 LANGUAGES CXX)
include(CheckCXXSourceCompiles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
enable_testing()

# Fetch GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/6dae7eb4a5c3a169f3e298392bff4680224aa94a.zip
)

if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

check_cxx_source_compiles("
#include <format>
int main() {
    std::format(\"{}\", 42);
    return 0;
}
" STD_FORMAT_SUPPORT)

if (ENABLE_TESTING)
	add_subdirectory(test)
endif(ENABLE_TESTING)

add_library(ZedOne STATIC include/server.hpp src/server.cpp)
target_include_directories(ZedOne PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(App src/main.cpp)
target_include_directories(App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(App PRIVATE ZedOne)

