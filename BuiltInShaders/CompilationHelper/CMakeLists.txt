cmake_minimum_required (VERSION 3.16)

file(GLOB VERTEX_SHADERS CONFIGURE_DEPENDS ../VS*.hlsl)
file(GLOB PIXEL_SHADERS CONFIGURE_DEPENDS ../PS*.hlsl)

macro (compile_shaders) 
    set(oneValueArgs SHADER_TYPE SHADER_MODEL)
    set(multiValueArgs SHADER_FILES)
    cmake_parse_arguments(COMPILE_SHADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    string(TOUPPER "${COMPILE_SHADERS_SHADER_TYPE}" ENTRY_POINT)
    string(APPEND ENTRY_POINT "Main")
    string(TOLOWER "${COMPILE_SHADERS_SHADER_TYPE}" TARGET_PROFILE)
    string(APPEND TARGET_PROFILE "_" ${COMPILE_SHADERS_SHADER_MODEL})
    
    foreach(FILE ${COMPILE_SHADERS_SHADER_FILES})
        get_filename_component(FILE_WE ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${GENERATED_DIR}/Shaders/${FILE_WE}.h
            COMMAND dxc.exe -nologo -E ${ENTRY_POINT} -T ${TARGET_PROFILE} -Zs -Fh ${GENERATED_DIR}/Shaders/${FILE_WE}.h ${FILE} -Vn g_${FILE_WE}
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${COMPILE_SHADERS_SHADER_TYPE}: ${FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo "dxc.exe -nologo -E ${ENTRY_POINT} -T ${TARGET_PROFILE} -Zs -Fh ${GENERATED_DIR}/Shaders/${FILE_WE}.h ${FILE} -Vn g_${FILE_WE}"
            MAIN_DEPENDENCY ${FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)
        list(APPEND COMPILED_SHADERS ${GENERATED_DIR}/Shaders/${FILE_WE}.h)
    endforeach()
endmacro()

compile_shaders(SHADER_TYPE "vs" SHADER_MODEL "6_0" SHADER_FILES ${VERTEX_SHADERS})
compile_shaders(SHADER_TYPE "ps" SHADER_MODEL "6_0" SHADER_FILES ${PIXEL_SHADERS})

message("${COMPILED_SHADERS}")
add_custom_target(shaderCompilationHelper
    DEPENDS ${COMPILED_SHADERS}) 