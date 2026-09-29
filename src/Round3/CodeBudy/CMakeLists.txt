cmake_minimum_required(VERSION 3.16)

project(TankGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 使用 Win32 宽字符 API（wWinMain / L"..."）
add_compile_definitions(UNICODE _UNICODE)

if(MSVC)
    # 源码为 UTF-8 编码，确保中文字符串/注释正确编译
    add_compile_options(/utf-8)
endif()

add_executable(TankGame WIN32
    src/main.cpp
    src/Renderer.cpp
    src/TextureLoader.cpp
    src/Sprites.cpp
    src/Game.cpp
    src/Audio.cpp
)

target_include_directories(TankGame PRIVATE src)

target_link_libraries(TankGame PRIVATE
    d3d11
    d3dcompiler
    dxgi
    winmm
    windowscodecs
    user32
    gdi32
    ole32
)

# 让 VS 调试时以工程根目录为工作目录，便于定位 resources
set_target_properties(TankGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# 构建后将 resources 复制到可执行文件目录，使 exe 可独立运行
add_custom_command(TARGET TankGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:TankGame>/resources"
)
