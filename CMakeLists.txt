cmake_minimum_required(VERSION 3.25.1)

project(sealloc)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

option(BuildType "Type of the build (Release, Debug)" "Release")
option(Log "Enable logging" "OFF")
option(Tests "Build tests (ON/OFF)" "OFF")
option(Assert "Build with assertions (ON/OFF)" "OFF")
option(Statistics "Build with statistics (ON/OFF)" "OFF")
option(Memtags "Build on ARM64 v8.5 MTE (ON/OFF)" "OFF")

# Build sources
add_subdirectory(./src)

if(Tests MATCHES "ON")
    # Install GoogleTest
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Build tests
    add_subdirectory(./test)
endif()
