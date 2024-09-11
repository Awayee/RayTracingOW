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