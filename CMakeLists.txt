cmake_minimum_required(VERSION 3.20)
project(casio_ui_sim LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# ---- Real U8g2 (pinned). ------------------------------------------------
# Offline / already have it?  cmake -DFETCHCONTENT_SOURCE_DIR_U8G2=/path/to/u8g2
include(FetchContent)
FetchContent_Declare(u8g2
    URL https://github.com/olikraus/u8g2/archive/refs/tags/2.36.19.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(u8g2)   # provides static library target "u8g2"

add_library(calc_core STATIC
    src/calc_engine.cpp
    src/calc_features.cpp)
target_include_directories(calc_core PUBLIC src)

add_executable(casio_ui_sim
    src/main.cpp
    src/calculator.cpp
    src/natural.cpp
    src/natural_draw.cpp
    src/display.cpp
    src/lcd_hal.c)
target_link_libraries(casio_ui_sim PRIVATE calc_core u8g2 PkgConfig::SDL2)

if(BUILD_TESTING)
    add_executable(calc_engine_test
        tests/calc_engine_test.cpp)
    target_link_libraries(calc_engine_test PRIVATE calc_core)
    add_test(NAME calc_engine_test COMMAND calc_engine_test)

    add_executable(calc_features_test
        tests/calc_features_test.cpp)
    target_link_libraries(calc_features_test PRIVATE calc_core)
    add_test(NAME calc_features_test COMMAND calc_features_test)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(casio_ui_sim PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>)
endif()
