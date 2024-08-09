#include "debugger.h"
#include "strconv.h"

namespace debugger
{
    using namespace std;
    using namespace strconv;

    vector<DllInfo> dllList; // 실제 벡터 객체의 인스턴스화
   
    PcCollectionManager pcManager; // pcCollection 벡터의 안전한 접근을 위한 pcCollectionManager 클래스 인스턴스화

    // pcCollection 벡터에 pc 데이터를 입력하는 메서드
    void PcCollectionManager::pushPcInfo(PVOID pc, DWORD threadId)
    {
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.push({ pc, threadId }); // 추적할 pc와 threadID를 PcInfo 구조체 벡터에 저장
    }

    // pcCollection 벡터에서 안전하게 pc 정보를 읽어오는 메서드
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
        // 메모리 초기화
        cbNeeded = NULL;
        ZeroMemory(&debugEvent, sizeof(debugEvent)); // 디버깅 이벤트 메모리 초기화
        ZeroMemory(hMods, sizeof(hMods)); // 핸들 배열 초기화
        ZeroMemory(&modInfo, sizeof(modInfo)); // 모듈 정보 구조체 초기화
        ZeroMemory(&si, sizeof(STARTUPINFO)); // si 구조체 메모리 초기화
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)); // pi 구조체 메모리 초기화

        // 기본값 할당
        si.cb = sizeof(STARTUPINFO); // 기본값 사이즈로 초기화

        // 디버깅할 프로세스 생성
        if (!CreateProcess(NULL, const_cast<LPWSTR>(tstringToLPWSTR(cmdLine)), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS, NULL, NULL, &si, &pi))
        {
            fprintf(stderr, "Error creating process: %d\n", GetLastError());
            throw runtime_error("Error creating process");
        }
        hProcess = pi.hProcess;
    }

    Debug::~Debug() // Debug 클래스 소멸자
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        //sharedObj.reset(); // 공유 객체 삭제
    }

    // step-by-step 실행을 위한 context의 trap flag 설정
    void Debug::SetTrapFlag(HANDLE hThread)
    {
        DWORD threadID = GetThreadId(hThread);
        CONTEXT ctx = { 0 };

        if ((threadID != NULL) && (contextMap.find(threadID) == contextMap.end()))
        {
            ctx.ContextFlags = CONTEXT_CONTROL; // context의  control register 영역

            if (!GetThreadContext(hThread, &ctx)) // 현재 컨텍스트를 불러옴
            {
                fprintf(stderr, "Failed to get thread context: %d\n", GetLastError());
                return;
            }

            ctx.EFlags |= 0x100; // TF, Trap Flag 설정

            if (!SetThreadContext(hThread, &ctx)) // TF를 설정한 컨텍스트 업데이트
            {
                fprintf(stderr, "Failed to set thread context: %d\n", GetLastError());
            }

            contextMap.emplace(threadID, hThread); // 설정한 컨텍스트 저장
        }
    }

    void Debug::loop(atomic_bool* isDebuggerOn)
    {
        TCHAR szModName[MAX_PATH];
        CONTEXT ctx = { 0 };

        while (continueDebugging && WaitForDebugEvent(&debugEvent, INFINITE))
        {
            *isDebuggerOn = true;
            switch (debugEvent.dwDebugEventCode)
            {
                // trap flag 설정 시 해당 case에 걸림
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) // trap flag  exception 확인시
                {
                    auto context = contextMap.find(debugEvent.dwThreadId); // 해당 threadID 검색
                    if (context != contextMap.end())
                    {
                        ctx.ContextFlags = CONTEXT_CONTROL;
                        if (GetThreadContext(context->second, &ctx)) // map에 저장된 핸들의 context를 가져옴
                        {
                            pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                            ctx.EFlags |= 0x100; // TF 설정
                            SetThreadContext(context->second, &ctx); // context 로드
                        }
                    }
                }
                break;

                // 프로세스,스레드 생성 이벤트 발생 시
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
                            dllList.push_back({ szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage });
                            // wprintf(L"%ls loaded..\n", szModName);
                        }
                    }
                }
                SetTrapFlag(debugEvent.u.CreateThread.hThread); // 첫 context TF 설정
            }
            break;

            case EXIT_PROCESS_DEBUG_EVENT:
                continueDebugging = FALSE; // 디버거(루프) 해제 
                break;
            }
            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
        }
        printf("collecting pc finished...\n");
        *isDebuggerOn = false;
    }
}
















