cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Set the project name
project(DuplicateFinder)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


if (MSVC)
  add_compile_options( "/W4" "/WX" "/MP" "/Zi" )
else()
  message(FATAL_ERROR "Only Windows is currently supported.")
endif()

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# Add the source directory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

