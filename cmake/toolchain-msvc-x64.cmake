# Native Windows x64 builds require an initialized MSVC developer environment.
# How to run: .\build.ps1 -Config Debug -Target all -Tests
if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "Use this toolchain on a Windows x64 MSVC host.")
endif()
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
