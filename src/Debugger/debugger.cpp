#include "debugger.h"

namespace debugger
{
    using namespace std;
    using namespace strconv;

    StrConv StrConv_;

    vector<LoadedDllInfo> LoadedDLLInfoList; 
   
    PcCollectionManager pcManager; 

    void PcCollectionManager::pushPcInfo(PVOID pc, DWORD threadId)
    {
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.push({ pc, threadId }); 
    }

    PcInfo PcCollectionManager::getPcInfo()
    {
        PcInfo temp = pcCollection.front();
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.pop();
       
        return temp;
    }

    bool PcCollectionManager::isEmpty()
    {
        return pcCollection.empty();
    }

    Debug::Debug(tstring  tCmdLine)
    {
        cbNeeded = 0;
        ZeroMemory(&debugEvent, sizeof(debugEvent));
        ZeroMemory(hMods, sizeof(hMods));
        ZeroMemory(&modInfo, sizeof(modInfo)); 
        ZeroMemory(&processStartupInfo, sizeof(STARTUPINFO)); 
        ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION)); 

        processStartupInfo.cb = sizeof(STARTUPINFO);
#ifdef UNICODE
        wstring cmdLine = tCmdLine;
#else
        wstring cmdLine = StrConv_.ansi2unicode(tCmdLine);
#endif
        if (!CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS, NULL, NULL, &processStartupInfo, &processInfo))
        {
            fprintf(stderr, "Error creating process: %d\n", GetLastError());
            throw runtime_error("Error creating process");
        }
        hProcess = processInfo.hProcess;
    }

    Debug::~Debug() 
    {
        TerminateProcess(processInfo.hProcess, 0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }

    void Debug::SetTrapFlag(HANDLE hThread)
    {
        DWORD threadID = GetThreadId(hThread);
        CONTEXT ctx = { 0 };

        if ((threadID != 0) && (targetContextMap.find(threadID) == targetContextMap.end()))
        {
            ctx.ContextFlags = CONTEXT_CONTROL; 

            if (!GetThreadContext(hThread, &ctx)) 
            {
                fprintf(stderr, "Failed to get thread context: %d\n", GetLastError());
                return;
            }

            ctx.EFlags |= 0x100; 

            if (!SetThreadContext(hThread, &ctx)) 
            {
                fprintf(stderr, "Failed to set thread context: %d\n", GetLastError());
            }

            targetContextMap.emplace(threadID, hThread);
        }
    }

    void Debug::debuggingLoop(std::atomic_bool* isDebuggerOn)
    {
        
        TCHAR szModName[MAX_PATH];
        CONTEXT ctx = { 0 };

        while (isDebugging && WaitForDebugEvent(&debugEvent, INFINITE))
        {
            *isDebuggerOn = true;
            switch (debugEvent.dwDebugEventCode)
            {
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) 
                {
                    auto context = targetContextMap.find(debugEvent.dwThreadId); 
                    if (context != targetContextMap.end())
                    {
                        ctx.ContextFlags = CONTEXT_CONTROL;
                        if (GetThreadContext(context->second, &ctx))
                        {
                            pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                            ctx.EFlags |= 0x100; 
                            SetThreadContext(context->second, &ctx); 
                        }
                    }
                }
                break;

            case CREATE_PROCESS_DEBUG_EVENT:
            case CREATE_THREAD_DEBUG_EVENT:
            {
                if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
                {
                    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                    {
                        if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))
                            && GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
                        {   
                            LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));
                            tcout << szModName << "is loaded" << endl;
                        }
                    }
                }
                SetTrapFlag(debugEvent.u.CreateThread.hThread);
            }
            break;

            case EXIT_PROCESS_DEBUG_EVENT:
                isDebugging = FALSE; 
                break;
            }
            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
        }
        printf("collecting pc finished...\n");
        *isDebuggerOn = false;
    }
}
















