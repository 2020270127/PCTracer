#include "debugger.h"
#include "strconv.h"
#include <iostream>

namespace debugger
{
    using namespace std;
    using namespace strconv;

    vector<DllInfo> dllList; // ???? ???? ????? ????????
   
    PcCollectionManager pcManager; // pcCollection ?????? ?????? ?????? ???? pcCollectionManager ????? ????????

    // pcCollection ????? pc ??????? ?????? ?????
    void PcCollectionManager::pushPcInfo(PVOID pc, DWORD threadId)
    {
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.push({ pc, threadId }); // ?????? pc?? threadID?? PcInfo ????? ????? ????
    }

    // pcCollection ??????? ??????? pc ?????? ??????? ?????
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

    Debug::Debug(tstring  cmdLine)
    {
        // ??? ????
        cbNeeded = 0;
        ZeroMemory(&debugEvent, sizeof(debugEvent)); // ????? ???? ??? ????
        ZeroMemory(hMods, sizeof(hMods)); // ??? ??? ????
        ZeroMemory(&modInfo, sizeof(modInfo)); // ??? ???? ????? ????
        ZeroMemory(&si, sizeof(STARTUPINFO)); // si ????? ??? ????
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)); // pi ????? ??? ????

        // ???? ???
        si.cb = sizeof(STARTUPINFO); // ???? ??????? ????

        // ??????? ???????? ????
        if (!CreateProcessW(NULL, const_cast<LPWSTR>(tstringToLPWSTR(cmdLine)), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS, NULL, NULL, &si, &pi))
        {
            fprintf(stderr, "Error creating process: %d\n", GetLastError());
            throw runtime_error("Error creating process");
        }
        hProcess = pi.hProcess;
    }

    Debug::~Debug() // Debug ????? ?????
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        //sharedObj.reset(); // ???? ??? ????
    }

    // step-by-step ?????? ???? context?? trap flag ????
    void Debug::SetTrapFlag(HANDLE hThread)
    {
        DWORD threadID = GetThreadId(hThread);
        CONTEXT ctx = { 0 };

        if ((threadID != 0) && (contextMap.find(threadID) == contextMap.end()))
        {
            ctx.ContextFlags = CONTEXT_CONTROL; // context??  control register ????

            if (!GetThreadContext(hThread, &ctx)) // ???? ???????? ?????
            {
                fprintf(stderr, "Failed to get thread context: %d\n", GetLastError());
                return;
            }

            ctx.EFlags |= 0x100; // TF, Trap Flag ????

            if (!SetThreadContext(hThread, &ctx)) // TF?? ?????? ?????? ???????
            {
                fprintf(stderr, "Failed to set thread context: %d\n", GetLastError());
            }

            contextMap.emplace(threadID, hThread); // ?????? ?????? ????
        }
    }

    void Debug::loop(std::atomic_bool* isDebuggerOn)
    {
        
        TCHAR szModName[MAX_PATH];
        CONTEXT ctx = { 0 };

        while (continueDebugging && WaitForDebugEvent(&debugEvent, INFINITE))
        {
            *isDebuggerOn = true;
            switch (debugEvent.dwDebugEventCode)
            {
                // trap flag ???? ?? ??? case?? ???
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) // trap flag  exception ?????
                {
                    auto context = contextMap.find(debugEvent.dwThreadId); // ??? threadID ???
                    if (context != contextMap.end())
                    {
                        ctx.ContextFlags = CONTEXT_CONTROL;
                        if (GetThreadContext(context->second, &ctx)) // map?? ????? ????? context?? ??????
                        {
                            pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                            ctx.EFlags |= 0x100; // TF ????
                            SetThreadContext(context->second, &ctx); // context ????
                        }
                    }
                }
                break;

                // ????????,?????? ???? ???? ??? ??
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
                            std::wstring wModName(szModName, szModName + strlen(szModName));
    

                            dllList.push_back(DllInfo(wModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));
                            wcout << wModName << L"is loaded" << endl;
                        }
                    }
                }
                SetTrapFlag(debugEvent.u.CreateThread.hThread); // ? context TF ????
            }
            break;

            case EXIT_PROCESS_DEBUG_EVENT:
                continueDebugging = FALSE; // ?????(????) ???? 
                break;
            }
            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
        }
        printf("collecting pc finished...\n");
        *isDebuggerOn = false;
    }
}
















