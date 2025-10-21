cmake_minimum_required(VERSION 3.19)

set(TARGET_NAME "RayTracingApp")
file(GLOB COMMON_FILES 
 "${SOURCE_PATH}/*.cpp"
 "${SOURCE_PATH}/Core/*.h"
 "${SOURCE_PATH}/Core/*.cpp"
 "${SOURCE_PATH}/Math/*.h"
 "${SOURCE_PATH}/Math/*.cpp"
 "${SOURCE_PATH}/RHI/*.h"
 "${SOURCE_PATH}/RHI/*.cpp"
 "${SOURCE_PATH}/RHI/Common/*.h"
 "${SOURCE_PATH}/RHI/Common/*.cpp"
 "${SOURCE_PATH}/Resource/*.h"
 "${SOURCE_PATH}/Resource/*.cpp"
 "${SOURCE_PATH}/RayTracing/*.h"
 "${SOURCE_PATH}/RayTracing/*.cpp"
 )

if(PLATFORM_WINDOWS)
    file(GLOB PLATFORM_FILES
    "${SOURCE_PATH}/RHI/Windows/*.h"
    "${SOURCE_PATH}/RHI/Windows/*.cpp"
    )
endif()

source_group(TREE ${SOURCE_PATH} FILES ${COMMON_FILES} ${PLATFORM_FILES})
add_executable(${TARGET_NAME} ${COMMON_FILES} ${PLATFORM_FILES})

if(PLATFORM_WINDOWS)
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
if(PLATFORM_WINDOWS)
    target_include_directories(${TARGET_NAME} PRIVATE ${THIRD_PARTY_PATH}/DirectX-Headers/include)
endif()

# vulkan, glfw, dxc
target_include_directories(${TARGET_NAME} PRIVATE
    ${THIRD_PARTY_PATH}/Vulkan/include
    ${THIRD_PARTY_PATH}/glfw/include
    ${THIRD_PARTY_PATH}/stbimage/
 )

if(PLATFORM_WINDOWS)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/DirectXShaderCompiler/inc
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/Vulkan/lib/Win64/vulkan-1.lib
        ${THIRD_PARTY_PATH}/glfw/lib-vc2019/glfw3.lib
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

elseif(PLATFORM_LINUX)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/DirectXShaderCompiler/inc
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${THIRD_PARTY_PATH}/Vulkan/lib/Linux/libvulkan.so.1
        ${THIRD_PARTY_PATH}/glfw/lib-linux/libglfw3.a
        ${THIRD_PARTY_PATH}/DirectXShaderCompiler/lib/linux/libdxcompiler.so
        ${THIRD_PARTY_PATH}/DirectXShaderCompiler/lib/linux/libdxil.so
    )

    if(OpenMP_CXX_FOUND)
        target_link_libraries(${TARGET_NAME} PUBLIC OpenMP::OpenMP_CXX)
        message(STATUS "OpenMP found and linked")
    else()
        message(WARNING "OpenMP not found")
    endif()
endif()
