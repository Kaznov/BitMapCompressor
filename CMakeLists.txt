cmake_minimum_required(VERSION 3.12)

project(rainworld_clock VERSION 0.0.1)

include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)
include(cmake/CompilerWarnings.cmake)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)

if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 23)
endif()

add_executable(bitmap_compressor
    ${PROJECT_SOURCE_DIR}/src/bitmap_compressor.cpp
    ${PROJECT_SOURCE_DIR}/src/bitmap_loader.cpp)
target_link_libraries(bitmap_compressor PRIVATE project_warnings)
