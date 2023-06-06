cmake_minimum_required (VERSION 3.16)
include(FetchContent)

# Add source to this project's executable.
file(GLOB SRC CONFIGURE_DEPENDS *.cpp)
file(GLOB INC ../inc/*.h ../inc/*.hpp ../interface/*.h)
file(GLOB INL ../inc/*.inl *.inl)

add_library (CM_HlslShaderToy SHARED ${SRC} ${INC} ${INL} CM_HlslShaderToy.def)

FetchContent_Declare(
    d3dx12
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        9ca4839a1b49aeac56c86036212dc035b1cf4a09
)
FetchContent_MakeAvailable(d3dx12)

target_link_libraries(CM_HlslShaderToy DirectX-Headers)

add_dependencies(CM_HlslShaderToy BuiltInShaders)
target_link_libraries(CM_HlslShaderToy BuiltInShaders)

#set_property(TARGET CM_HlslShaderToy
#    PROPERTY VS_PACKAGE_REFERENCES "Microsoft.Direct3D.DXC_1.7.2212"
#)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../packages.config ${CMAKE_CURRENT_BINARY_DIR}/packages.config COPYONLY)
add_library(dxcompiler INTERFACE IMPORTED GLOBAL)
set_target_properties(dxcompiler PROPERTIES INTERFACE_LINK_LIBRARIES "${CMAKE_BINARY_DIR}/packages/Microsoft.Direct3D.DXC.1.7.2212.36/build/native/Microsoft.Direct3D.DXC.targets")
target_link_libraries(CM_HlslShaderToy dxcompiler)

set(CMAKE_${lang}_LINK_LIBRARY_USING_delayload "/DELAYLOAD:dxil.dll")

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
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CM_HlslShaderToy>
        %APPDATA%/ColumnMode/Plugins/$<TARGET_FILE_BASE_NAME:CM_HlslShaderToy>/$<TARGET_FILE_NAME:CM_HlslShaderToy>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE_DIR:CM_HlslShaderToy>/dxil.dll
        %APPDATA%/ColumnMode/Plugins/$<TARGET_FILE_BASE_NAME:CM_HlslShaderToy>/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE_DIR:CM_HlslShaderToy>/dxcompiler.dll
        %APPDATA%/ColumnMode/Plugins/$<TARGET_FILE_BASE_NAME:CM_HlslShaderToy>/dxcompiler.dll
)