#include "debugger.h"
#include <tchar.h>
#include <format>

#define SPLITLINE "\n--------------------------------------------------------------------------------------------------------------------------\n"
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
        
        if (!CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS , NULL, NULL, &processStartupInfo, &processInfo))
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



    DWORD GetSizeOfImageFromFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::in);
        if (!file)
        {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return 0;
        }

        // DOS 헤더 읽기
        IMAGE_DOS_HEADER dosHeader;
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
        if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
            std::cerr << "Invalid DOS header in file: " << filePath << std::endl;
            return 0;
        }

        // NT 헤더로 이동
        file.seekg(dosHeader.e_lfanew, std::ios::beg);

        // NT 헤더 읽기
        IMAGE_NT_HEADERS ntHeaders;
        file.read(reinterpret_cast<char*>(&ntHeaders), sizeof(ntHeaders));
        if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        {
            std::cerr << "Invalid NT header in file: " << filePath << std::endl;
            return 0;
        }

        // SizeOfImage 반환
        return ntHeaders.OptionalHeader.SizeOfImage;
    }

bool isInProcessRange = true;  // 프로세스 내부 흐름을 추적하는 플래그


// 전역 변수 선언
PVOID g_baseAddr = nullptr;
SIZE_T g_imageSize = 0;
int createdThreadCount = 0;

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
                        // RIP가 프로세스 메모리 영역을 벗어나는지 확인
                        if ((PVOID)ctx.Rip < g_baseAddr || (PVOID)ctx.Rip >= (PBYTE)g_baseAddr + g_imageSize)
                        {
                            // 프로세스 영역을 벗어난 경우
                            if (isInProcessRange)
                            {
                                // 처음으로 프로세스 영역을 벗어났을 때만 기록
                                pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                                isInProcessRange = false;  // 플래그를 FALSE로 설정하여 프로세스 영역을 벗어났음을 표시
                            }
                        }
                        else
                        {
                            // 프로세스 영역 내에 있는 경우
                            isInProcessRange = true;  // 플래그를 TRUE로 설정하여 프로세스 영역 내에 있음을 표시
                        }

                        // 트랩 플래그를 설정하여 다음 명령어를 단일 스텝으로 실행
                        ctx.EFlags |= 0x100;
                        SetThreadContext(context->second, &ctx);
                    }
                }
                else
                {
                    tcout << "Context Invalid " << debugEvent.dwThreadId << endl;
                }
            }
            break;

        case CREATE_PROCESS_DEBUG_EVENT:
        {
            tcout << format(_T("Main process created\n"), createdThreadCount);
            SetTrapFlag(debugEvent.u.CreateProcessInfo.hThread);
        }
        break;
        
        case CREATE_THREAD_DEBUG_EVENT:
        {
            createdThreadCount++;
            tcout << format(_T("\nThread {} created\n"), createdThreadCount);
            if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_64BIT))
            {
                if (GetModuleFileNameEx(hProcess, hMods[0], szModName, sizeof(szModName) / sizeof(TCHAR))
                    && GetModuleInformation(hProcess, hMods[0], &modInfo, sizeof(modInfo)))
                {
                    if (g_baseAddr == nullptr) // 메인 모듈은 항상 첫번째로 로드
                    {
                        g_baseAddr = modInfo.lpBaseOfDll;
                        g_imageSize = modInfo.SizeOfImage;
                        tcout << format(_T("\nMain process's memory address range\n    0x{:x} ~ 0x{:x}, \n"), reinterpret_cast<uintptr_t>(g_baseAddr), reinterpret_cast<uintptr_t>(g_baseAddr) + g_imageSize);
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));  
                    }      
                }  
            }
            
            SetTrapFlag(debugEvent.u.CreateThread.hThread);  // 트랩 플래그를 설정
        }
        break;

        case LOAD_DLL_DEBUG_EVENT:
        {
            TCHAR szModName[MAX_PATH] = { 0 };
            DWORD sizeOfImage;
            HMODULE hMod = nullptr;
            

            if(debugEvent.u.LoadDll.hFile)
            {
                // Get the module file name
                if (GetFinalPathNameByHandleA(debugEvent.u.LoadDll.hFile, szModName, MAX_PATH, 0))
                {
                    // \\?\로 시작하는 경로를 처리
                    std::string path(szModName);
                    if (path.compare(0, 4, "\\\\?\\") == 0)
                    {
                        path = path.substr(4); // \\?\를 제거
                        strncpy(szModName, path.c_str(), MAX_PATH); // szModName에 다시 저장하여 C-스트링으로 유지
                    }


                    

                    if (GetModuleInformation(debugEvent.u.LoadDll.hFile, hMod, &modInfo, sizeof(modInfo)))
                    {
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));
                        tcout << format(_T("\nLoaded DLL Information By Target Process\n    Module:{}, Base Address:{}, Size Of Image:{}bytes\n"), szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage);
                    }
                   else
                    {
                        sizeOfImage = GetSizeOfImageFromFile(path);
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, debugEvent.u.LoadDll.lpBaseOfDll, sizeOfImage));
                        tcout << format(_T("\nLoaded DLL Information By Target Process, DLL path\n  Module:{}, Base Address:{}, Size Of Image:{}bytes\n"), szModName, debugEvent.u.LoadDll.lpBaseOfDll, sizeOfImage);
                    }
                }
            }
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
















