cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
set(TARGET_NAME "Source")
file(GLOB_RECURSE H_FILES "*.h")
file(GLOB_RECURSE CPP_FILES "*.cpp")
source_group(TREE ${SOURCE_PATH} FILES ${H_FILES} ${CPP_FILES})
add_executable(${TARGET_NAME} ${H_FILES} ${CPP_FILES})
target_link_libraries(${TARGET_NAME} PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    D3DCompiler.lib
    winmm.lib
)

target_include_directories(${TARGET_NAME} PRIVATE ${SOURCE_PATH})
# dx headers
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY_PATH}/DirectX-Headers/include)

# vulkan, glfw, dxc
target_include_directories(${TARGET_NAME} PRIVATE
 ${THIRD_PARTY_PATH}/Vulkan/include
 ${THIRD_PARTY_PATH}/glfwWin64/include
 ${THIRD_PARTY_PATH}/DirectXShaderCompiler/inc
 )

target_link_libraries(${TARGET_NAME} PRIVATE
 ${THIRD_PARTY_PATH}/Vulkan/lib/Win64/vulkan-1.lib
 ${THIRD_PARTY_PATH}/glfwWin64/lib-vc2019/glfw3.lib
 ${THIRD_PARTY_PATH}/DirectXShaderCompiler/lib/x64/dxcompiler.lib
 )

 # Copy dlls
 set(COPY_DLLS
    ${THIRD_PARTY_PATH}/DirectXShaderCompiler/bin/x64/dxcompiler.dll
    ${THIRD_PARTY_PATH}/DirectXShaderCompiler/bin/x64/dxil.dll
)

foreach(COPY_DLL ${COPY_DLLS})
    add_custom_command(TARGET ${TARGET_NAME}
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${COPY_DLL}
        $<TARGET_FILE_DIR:${TARGET_NAME}>)
endforeach()
