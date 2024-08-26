#pragma once
#include <windows.h>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <queue>
#include <atomic>
#include <iostream>

#include "strconv.h"

namespace debugger
{
    using namespace std;
    //using namespace strconv;
    extern strconv::StrConv StrConv_;

    static unordered_map<DWORD, HANDLE> targetContextMap; 

    struct LoadedDllInfo {
        tstring name;
        LPVOID baseAddr;
        DWORD size;
    };

    struct PcInfo {
        PVOID pc;
        DWORD threadId;
    };

    extern vector<LoadedDllInfo> LoadedDLLInfoList; 

    class PcCollectionManager
    {
    private:
        queue<PcInfo> pcCollection;
        mutex pc_mtx;

    public:
        PcCollectionManager() {};
        ~PcCollectionManager() {};
        void pushPcInfo(PVOID pc, DWORD threadId);
        PcInfo getPcInfo();
        bool isEmpty();
    } ;
    extern PcCollectionManager pcManager;

    class Debug
    {
    private:
        DEBUG_EVENT debugEvent;
        PROCESS_INFORMATION processInfo; 
        STARTUPINFOW processStartupInfo; 
        BOOL isDebugging = TRUE;
        HANDLE hProcess; 
        HMODULE hMods[1024];  
        DWORD cbNeeded;
        MODULEINFO modInfo; 

    private:
        void SetTrapFlag(HANDLE hThread); 
    public:
        Debug(tstring  cmdLine); 
        ~Debug(); 
        void loop(std::atomic_bool* isDebuggerOn); 
    };
}

