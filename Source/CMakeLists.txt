cmake_minimum_required(VERSION 3.19)

set(TARGET_NAME "RayTracingApp")
file(GLOB COMMON_FILES 
 "${SOURCE_PATH}/*.cpp"
 "${SOURCE_PATH}/Core/*.h"
 "${SOURCE_PATH}/Core/*.cpp"
 "${SOURCE_PATH}/Math/*.h"
 "${SOURCE_PATH}/Math/*.cpp"
 "${SOURCE_PATH}/Renderer/*.h"
 "${SOURCE_PATH}/Renderer/*.cpp"
 "${SOURCE_PATH}/Renderer/Common/*.h"
 "${SOURCE_PATH}/Renderer/Common/*.cpp"
 "${SOURCE_PATH}/RayTracing/*.h"
 "${SOURCE_PATH}/RayTracing/*.cpp"
 )

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    file(GLOB PLATFORM_FILES
    "${SOURCE_PATH}/Renderer/Windows/*.h"
    "${SOURCE_PATH}/Renderer/Windows/*.cpp"
    )
endif()

source_group(TREE ${SOURCE_PATH} FILES ${COMMON_FILES} ${PLATFORM_FILES})
add_executable(${TARGET_NAME} ${COMMON_FILES} ${PLATFORM_FILES})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${TARGET_NAME} PRIVATE
        d3d12.lib
        dxgi.lib
        dxguid.lib
        D3DCompiler.lib
        winmm.lib
    )
endif()

target_include_directories(${TARGET_NAME} PRIVATE ${SOURCE_PATH})
# dx headers
target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY_PATH}/DirectX-Headers/include)

# vulkan, glfw, dxc
target_include_directories(${TARGET_NAME} PRIVATE
    ${THIRD_PARTY_PATH}/Vulkan/include
    ${THIRD_PARTY_PATH}/glfwWin64/include
 )

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/DirectXShaderCompiler/inc
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/Vulkan/lib/Win64/vulkan-1.lib
        ${THIRD_PARTY_PATH}/glfwWin64/lib-vc2019/glfw3.lib
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

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_include_directories(${TARGET_NAME} PRIVATE
         ${THIRD_PARTY_PATH}/DirectXShaderCompilerLinux/include/dxc
    )
     target_link_libraries(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/Vulkan/lib/Win64/vulkan-1.so
        ${THIRD_PARTY_PATH}/glfwWin64/lib-vc2019/glfw3.so
        ${THIRD_PARTY_PATH}/DirectXShaderCompilerLinux/lib/dxcompiler.so
        ${THIRD_PARTY_PATH}/DirectXShaderCompilerLinux/lib/dxil.so
    )
endif()
