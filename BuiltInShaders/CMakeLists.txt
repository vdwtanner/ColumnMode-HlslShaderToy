cmake_minimum_required (VERSION 3.16)

make_directory(${GENERATED_DIR}/Shaders)
add_subdirectory(CompilationHelper)

add_library(BuiltInShaders INTERFACE)
target_include_directories(BuiltInShaders INTERFACE ${GENERATED_DIR}/Shaders)

add_dependencies(BuiltInShaders shaderCompilationHelper)