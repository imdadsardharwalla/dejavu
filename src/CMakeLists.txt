set(target dejavu)

message(STATUS "Adding target: ${target}")

file(GLOB HXX_FILES ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
file(GLOB CXX_FILES ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

add_executable(${target} ${CXX_FILES} ${HXX_FILES})

target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party)
