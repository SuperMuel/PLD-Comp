cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PLD-Comp VERSION 0.1 DESCRIPTION "Simple C-Compiler")

list(APPEND DIRS generated)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp" "CodeGenVisitor.cpp" "DebugVisitor.cpp" "main.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} generated)
include_directories("/usr/include/antlr4-runtime")

add_executable(ifcc ${SOURCES})
target_link_libraries(ifcc "/usr/lib/x86_64-linux-gnu/libantlr4-runtime.a")
