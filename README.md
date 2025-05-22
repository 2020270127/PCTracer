# PCTracer & DLLParser

This repository contains two command-line tools designed for Windows (x64):

- **DLLParser**  
  Parses PE (Portable Executable) files and stores metadata in a SQLite3 database.  
- **PCTracer**  
  Provides call tracing and logging functionality for a specified process.

---

## Requirements

- **CMake** 3.15 or higher  
- **C++** Compiler with C++20 support  
  - Windows: Visual Studio 2022 (x64)  

---

## Directory Structure

```
├── CMakeLists.txt          # Project configuration
├── README.md               # This document
├── src/
│   ├── sqlite3/            # Embedded SQLite3 source
│   ├── Debugger/           # Debugger library source
│   ├── DLLParser/          # DLLParser executable source
│   └── PCTracer/           # PCTracer executable source
└── build/                  # (Generated) Build output directory
```

---

## Build Instructions

1. **Create and enter the build directory**  
   ```bat
   mkdir build
   cd build
   ```

2. **Configure with CMake (Visual Studio 2022, x64)**  
   ```bat
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

3. **Build the project (Release configuration)**  
   ```bat
   cmake --build . --config Release
   ```

4. **Executable locations**  
   - `build/Release/DLLParser.exe`  
   - `build/Release/PCTracer.exe`  

---

## Command-Line Options

### DLLParser

**Options**:
```
  -d, --directory <path>    The target DLLs' directory (required)
  -n, --DBname <name>       The output database name without extension (default: DLL.db)
```

**Usage**:
```bat
DLLParser.exe -d C:\path -n MyDatabase
```

---

### PCTracer

**Options**:
```
  -t, --target_path <path>  Path to the target executable or process (required)
  -d, --db_path <path>      Path to the SQLite3 database file to record logs (required)
  -l, --log_level <level>   Log level: 1 for TEXT, 2 for DB (default: 1)
```

**Usage**:
```bat
PCTracer.exe -t C:\PathOfTarget.exe -d PathOfTargetDB.db
```

---

## Key Libraries

- **Sqlite3**  
  Embedded (static) — `src/sqlite3/sqlite3.c`  
- **Debugger**  
  Windows API call wrapping and logging library — `src/Debugger/*.cpp`

---

## Compilation Options

- **C++ Standard**: C++20  

---


