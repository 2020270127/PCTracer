#pragma once
#include <windows.h>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <queue>
#include <atomic>
#include "typedef.h"

namespace debugger
{
    using namespace std;

    static unordered_map<DWORD, HANDLE> contextMap; // ���α׷��� Context�� ������ ��

    // DLL ������ ������ ����ü
    struct DllInfo {
        wstring name;
        LPVOID baseAddr;
        DWORD size;
    };

    // ���� �帧�� ���� PC, threadID�� ������ ����ü
    struct PcInfo {
        PVOID pc;
        DWORD threadId;
    };

    extern vector<DllInfo> dllList; // DllInfo ����ü�� ������ ����

    // �����ϴ� pcCollection ���͸� �����ϰ� �����ϱ� ���� Ŭ����
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
        DEBUG_EVENT debugEvent; // ����� �̺�Ʈ ����ü
        PROCESS_INFORMATION pi; // ���μ��� ���� ����ü
        STARTUPINFO si; // ���μ��� ���� ���� ����ü
        BOOL continueDebugging = TRUE; // ����� ���� ����
        HANDLE hProcess; // EnumProcessModules�� ����� �˻��� ��� ���μ���
        HMODULE hMods[1024]; // EnumProcessModules�� ���μ����� �� ����� �ޱ� ���� �ڵ� �迭, �ִ� 1024���� ����� ���� (�⺻��) 
        DWORD cbNeeded; // ���� �ε�� ����� ũ��
        MODULEINFO modInfo; // ��� ���� ����ü
        void SetTrapFlag(HANDLE hThread); // pc ����� ���� context trap flag ���� �޼���
    public:
        Debug(tstring  cmdLine); // Debug Ŭ���� ������
        ~Debug(); // Debug Ŭ���� �Ҹ���
        void loop(atomic_bool* isDebuggerOn); // pc ����� ���� ����� ����
    };
}

