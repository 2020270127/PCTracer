#include "debugger.h"
#include <tchar.h>

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
    
    void EnableDebugPrivilege()
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;

        // ���� ���μ����� ��ū�� �����ɴϴ�.
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            // ����� ������ LUID�� �����ɴϴ�.
            LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;  // 1���� ������ �����մϴ�.
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // ���μ��� ��ū�� ����� ������ �����մϴ�.
            AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL);

            if (GetLastError() == ERROR_SUCCESS)
            {
                std::wcout << L"SeDebugPrivilege successfully enabled." << std::endl;
            }
            else
            {
                std::wcerr << L"Failed to enable SeDebugPrivilege." << std::endl;
            }

            CloseHandle(hToken);
        }
        else
        {
            std::wcerr << L"Failed to open process token." << std::endl;
        }
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
        EnableDebugPrivilege();
        if (!CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, NULL, NULL, &processStartupInfo, &processInfo))
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
                    tcout << "WWWWWWWWW thread id: " << debugEvent.dwThreadId << endl;
                }
            }
            break;

        case CREATE_PROCESS_DEBUG_EVENT:
        {
            hProcess = debugEvent.u.CreateProcessInfo.hProcess;
            tcout << "Process created, handle: " << hProcess << endl;

            
            
            // ���� �����忡 ���� SetTrapFlag ȣ��
            SetTrapFlag(debugEvent.u.CreateProcessInfo.hThread);

            // �߰� �ʱ�ȭ �ڵ�...
        }
        break;

        case CREATE_THREAD_DEBUG_EVENT:
        {
           
            if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_64BIT))
            {
                

                for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                {
                    if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))
                        && GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
                    {
                        if (!_tcsicmp(szModName, _T("C:\\Users\\Administrator\\source\\repos\\fuzzXpdf\\x64\\Debug\\HELLO.exe")))
                        {
                            g_baseAddr = modInfo.lpBaseOfDll;
                            g_imageSize = modInfo.SizeOfImage;
                            tcout << "Main Module Base Address: " << g_baseAddr << " Size: " << g_imageSize << " bytes" << endl;
                        }
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage));
                        tcout << szModName << " is loaded" << endl;
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

            if (debugEvent.u.LoadDll.hFile)
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

                    tcout << szModName << " is loaded" << endl;

                    if (path == "C:\\Users\\Administrator\\source\\repos\\fuzzXpdf\\x64\\Debug\\printhello.dll")
                    {
                        // ���� ��θ� ������� SizeOfImage ��������
                        sizeOfImage = GetSizeOfImageFromFile(path);
                        LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, debugEvent.u.LoadDll.lpBaseOfDll, sizeOfImage));
                        tcout << "Module: " << szModName << " Base Address: " << debugEvent.u.LoadDll.lpBaseOfDll << " SizeOfImage: " << sizeOfImage << " bytes" << endl;
                        // �߰� �۾� ����...
                    }

                    // LoadedDLLInfoList�� �߰� (CREATE_THREAD_DEBUG_EVENT ��İ� �����ϰ�)
                    // LoadedDLLInfoList.push_back(LoadedDllInfo(szModName, debugEvent.u.LoadDll.lpBaseOfDll, sizeOfImage));

                    //tcout << "Module: " << szModName << " Base Address: " << debugEvent.u.LoadDll.lpBaseOfDll << " SizeOfImage: " << sizeOfImage << " bytes" << endl;
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
















