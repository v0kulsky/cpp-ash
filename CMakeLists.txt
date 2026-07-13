cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
  src/music.cpp)

target_include_directories(${PROJECT_NAME}
  PUBLIC include)

target_link_libraries(${PROJECT_NAME}
  PRIVATE SDL3::SDL3)
