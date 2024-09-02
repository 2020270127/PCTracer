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

        // DOS ��� �б�
        IMAGE_DOS_HEADER dosHeader;
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
        if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
            std::cerr << "Invalid DOS header in file: " << filePath << std::endl;
            return 0;
        }

        // NT ����� �̵�
        file.seekg(dosHeader.e_lfanew, std::ios::beg);

        // NT ��� �б�
        IMAGE_NT_HEADERS ntHeaders;
        file.read(reinterpret_cast<char*>(&ntHeaders), sizeof(ntHeaders));
        if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        {
            std::cerr << "Invalid NT header in file: " << filePath << std::endl;
            return 0;
        }

        // SizeOfImage ��ȯ
        return ntHeaders.OptionalHeader.SizeOfImage;
    }

bool isInProcessRange = true;  // ���μ��� ���� �帧�� �����ϴ� �÷���


// ���� ���� ����
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
                        // RIP�� ���μ��� �޸� ������ ������� Ȯ��
                        if ((PVOID)ctx.Rip < g_baseAddr || (PVOID)ctx.Rip >= (PBYTE)g_baseAddr + g_imageSize)
                        {
                            // ���μ��� ������ ��� ���
                            if (isInProcessRange)
                            {
                                // ó������ ���μ��� ������ ����� ���� ���
                                pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                                isInProcessRange = false;  // �÷��׸� FALSE�� �����Ͽ� ���μ��� ������ ������� ǥ��
                            }
                        }
                        else
                        {
                            // ���μ��� ���� ���� �ִ� ���
                            isInProcessRange = true;  // �÷��׸� TRUE�� �����Ͽ� ���μ��� ���� ���� ������ ǥ��
                        }

                        // Ʈ�� �÷��׸� �����Ͽ� ���� ��ɾ ���� �������� ����
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
                    if (g_baseAddr == nullptr) // ���� ����� �׻� ù��°�� �ε�
                    {
                        g_baseAddr = modInfo.lpBaseOfDll;
                        g_imageSize = modInfo.SizeOfImage;
                        tcout << format(_T("\nMain process's memory address range\n    0x{:x} ~ 0x{:x}, \n"), reinterpret_cast<uintptr_t>(g_baseAddr), reinterpret_cast<uintptr_t>(g_baseAddr) + g_imageSize);
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));  
                    }      
                }  
            }
            
            SetTrapFlag(debugEvent.u.CreateThread.hThread);  // Ʈ�� �÷��׸� ����
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
                    // \\?\�� �����ϴ� ��θ� ó��
                    std::string path(szModName);
                    if (path.compare(0, 4, "\\\\?\\") == 0)
                    {
                        path = path.substr(4); // \\?\�� ����
                        strncpy(szModName, path.c_str(), MAX_PATH); // szModName�� �ٽ� �����Ͽ� C-��Ʈ������ ����
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
















