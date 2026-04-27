cmake_minimum_required(VERSION 4.1 FATAL_ERROR)

# Set the project name
project(dejavu)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Export the compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
  add_compile_options( "/W4" "/WX" "/MP" "/Zi" )
else()
  message(FATAL_ERROR "Only Windows is currently supported.")
endif()

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# Add the source directory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
