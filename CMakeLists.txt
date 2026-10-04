cmake_minimum_required(VERSION 3.20)
project(FolderTags LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(FolderTags SHARED
    src/dllmain.cpp
    src/context_menu.cpp
    src/tags.cpp
    src/draw.cpp
    src/tags_window.cpp
    src/index.cpp
    src/overlay.cpp
    src/FolderTags.def
)

target_compile_definitions(FolderTags PRIVATE UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_compile_options(FolderTags PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(FolderTags PRIVATE shlwapi shell32 ole32 gdiplus dwmapi user32 gdi32 advapi32)
