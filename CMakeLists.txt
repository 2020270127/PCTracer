cmake_minimum_required(VERSION 3.15)
cmake_policy(VERSION 3.15)

project(PCTracer VERSION 0.1.0 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent automatic 'lib' prefix on libraries
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Static SQLite3 library
add_library(Sqlite3 STATIC
    src/sqlite3/sqlite3.c
)
target_include_directories(Sqlite3 PUBLIC src/sqlite3)

# Static Debugger library
add_library(Debugger STATIC
    src/Debugger/strconv.cpp
    src/Debugger/debugger.cpp
)
target_include_directories(Debugger PUBLIC src/Debugger)

# DLLParser executable
add_executable(DLLParser
    src/DLLParser/main.cpp
    src/DLLParser/cmdparser.cpp
    src/DLLParser/dllparser.cpp
    src/DLLParser/logger.cpp
    src/DLLParser/pefile.cpp
    src/DLLParser/peparser.cpp
    src/DLLParser/peprint.cpp
    src/DLLParser/strconv.cpp
)
target_link_libraries(DLLParser PRIVATE Sqlite3)

# PCTracer executable
add_executable(PCTracer
    src/PCTracer/main.cpp
    src/PCTracer/cmdparser.cpp
    src/PCTracer/record.cpp
    src/PCTracer/logger.cpp
    src/PCTracer/strconv.cpp
)
target_link_libraries(PCTracer PRIVATE Debugger Sqlite3)

# Compiler warnings and flags
if(MSVC)
    target_compile_options(DLLParser PRIVATE /W4 /permissive-)
    target_compile_options(PCTracer PRIVATE /W4 /permissive-)
    target_compile_options(Sqlite3 PRIVATE /W4)
    target_compile_options(Debugger PRIVATE /W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Packaging
include(CPack)

# Build Instructions:
# 1. Create and enter build directory
#    mkdir build && cd build
# 2. Generate build system with Visual Studio 2022 (x64)
#    cmake .. -G "Visual Studio 17 2022" -A x64
# 3. Build in Release mode
#    cmake --build . --config Release
