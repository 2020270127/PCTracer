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
#include <Windows.h>

namespace debugger
{
    using namespace std;
    //using namespace strconv;
    extern strconv::StrConv StrConv_;

    static unordered_map<DWORD, HANDLE> targetContextMap; // 프로그램의 Context를 저장할 맵

    // DLL 정보를 저장할 구조체
    struct LoadedDllInfo {
        tstring name;
        LPVOID baseAddr;
        DWORD size;
    };

    // 실행 흐름에 따른 PC, threadID를 저장할 구조체
    struct PcInfo {
        PVOID pc;
        DWORD threadId;
    };

    extern vector<LoadedDllInfo> LoadedDLLInfoList; // DllInfo 구조체를 저장할 벡터

    // 공유하는 pcCollection 벡터를 안전하게 접근하기 위한 클래스
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
    };
    extern PcCollectionManager pcManager;

    class Debug
    {
    private:
        DEBUG_EVENT debugEvent; // 디버깅 이벤트 구조체
        PROCESS_INFORMATION processInfo; // 프로세스 정보 구조체
        STARTUPINFOW processStartupInfo; // 프로세스 시작 정보 구조체
        BOOL isDebugging = TRUE; // 디버깅 루프 상태
        HANDLE hProcess; // EnumProcessModules로 모듈을 검색할 대상 프로세스
        HMODULE hMods[1024]; // EnumProcessModules로 프로세스의 각 모듈을 받기 위한 핸들 배열, 최대 1024개의 모듈을 받음 (기본값) 
        DWORD cbNeeded; // 실제 로드된 모듈의 크기
        MODULEINFO modInfo; // 모듈 정보 구조체

    private:
        void SetTrapFlag(HANDLE hThread); // pc 기록을 위한 context trap flag 설정 메서드
    public:
        Debug(tstring  cmdLine); // Debug 클래스 생성자
        ~Debug(); // Debug 클래스 소멸자
        void loop(std::atomic_bool* isDebuggerOn); // pc 기록을 위한 디버깅 루프
    };
}

