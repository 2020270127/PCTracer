# PCTracer
- window executable binary's dll tracer using program counter  

## TODO
### Code Optimization
- 

## DOING
### 
-

## DONE
### 
- 

## How to build
### 
``` 
(cmake version 3.29.0-rc3)

mkdir build && cd build
1.  cmake ..
    VS solution build (vs2022)

2.  cmake -G "Unix Makefiles" ..
    make (GNU Make 3.81)
```

## Usage

### Parse Dll's Data
- DLLParser -d <target dll's directory> -n <DB's Name>
- #### ex) DLLParser -d C:\Windows\System32 -n MyDB

### Trace Target's dll
- PCTracer -d <DB's path> -t <target_exe's path> -l <log_level, 1 for text | 2 for sqlite3 DB>
- #### ex) PCTracer -d C:\Windows\DB\DLL.db -t C:\Windows\Target.exe -l 2

