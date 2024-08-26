cmake_minimum_required(VERSION 3.0.0)
project(PCTracer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 정적 라이브러리 생성 (lib)
add_library(
    Sqlite3 STATIC
    src/sqlite3/sqlite3.c
)

add_library(
    Debugger STATIC
    src/Debugger/strconv.cpp
    src/Debugger/debugger.cpp
)

# 애플리케이션 실행 파일 생성
add_executable(
    DLL_Parser
    src/DLLParser/main.cpp
    src/DLLParser/cmdparser.cpp
    src/DLLParser/dllparser.cpp
    src/DLLParser/logger.cpp
    src/DLLParser/pefile.cpp
    src/DLLParser/peparser.cpp
    src/DLLParser/peprint.cpp
    src/DLLParser/strconv.cpp
)

# 애플리케이션 실행 파일 생성
add_executable(
    PC_Tracer
    src/PCTracer/main.cpp
    src/PCTracer/cmdparser.cpp
    src/PCTracer/record.cpp
)

# 라이브러리 링크
target_link_libraries(DLL_Parser PRIVATE Sqlite3)
target_link_libraries(PC_Tracer PRIVATE Debugger Sqlite3)

add_compile_options(-Wall)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
