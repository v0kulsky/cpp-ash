cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHLOGGER_LEVEL 1 CACHE STRING
  "Compile-time logging level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR")

project(
  ash
  VERSION "0.1"
  DESCRIPTION "Not so compilcated audio engine library"
  LANGUAGES CXX)

### EXTERNAL LIBRARIES ###
find_package(SDL3 REQUIRED)

### ENTRY POINT ###
add_library(${PROJECT_NAME}
  STATIC
  src/engine.cpp
  src/data_source.cpp
  src/audio_stream.cpp)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE SHLOGGER_LEVEL=${SHLOGGER_LEVEL})

target_include_directories(${PROJECT_NAME}
  PUBLIC include
  PUBLIC third_party/minimp3)

target_link_libraries(${PROJECT_NAME}
  PRIVATE SDL3::SDL3)
