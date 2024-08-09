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

## ISSUE
### 
- 

## Usage

### Parse Dll's Data
- DLLParser -d <target dll's directory> 
- ex) DLLParser -d C:\Windows\System32

### Trace Target's dll
- PCTracer -d <DB's path> -t <target_exe's path> -l <log_level, 1 for text | 2 for sqlite3 DB>
- ex) PCTracer -d C:\Windows\DB\DLL.db -t C:\Windows\Target.exe -l 2

