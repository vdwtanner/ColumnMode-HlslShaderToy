cmake_minimum_required (VERSION 3.16)

# Add source to this project's executable.
file(GLOB SRC CONFIGURE_DEPENDS *.cpp)
file(GLOB INC ../inc/*.h ../inc/*.hpp ../interface/*.h)
file(GLOB INL ../inc/*.inl *.inl)

add_library (CM_HlslShaderToy SHARED ${SRC} ${INC} ${INL} CM_HlslShaderToy.def "ShaderToyWindow.cpp")
target_include_directories(CM_HlslShaderToy
    PRIVATE ../inc
    PRIVATE ../interface
    PRIVATE ./)

target_precompile_headers(CM_HlslShaderToy PUBLIC ../inc/pch.h)

source_group(Inlines FILES ${INL})

#Copy plugin dll to the Plugin folder
add_custom_command(
    TARGET CM_HlslShaderToy 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:CM_HlslShaderToy>
        %APPDATA%/ColumnMode/Plugins/$<TARGET_FILE_BASE_NAME:CM_HlslShaderToy>/$<TARGET_FILE_NAME:CM_HlslShaderToy>
)