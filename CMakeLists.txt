cmake_minimum_required (VERSION 3.16)

project ("CM_HlslShaderToy")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

make_directory(${CMAKE_CURRENT_SOURCE_DIR}/Generated)
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Generated")

# Include sub-projects.
add_subdirectory("BuiltInShaders")
add_subdirectory ("src")