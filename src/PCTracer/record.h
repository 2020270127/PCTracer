#pragma once
#include <atomic>
#include "typedef.h"

namespace record
{
	using namespace std;

	class RECORD
	{
	private:
		int log_type_; // �α� Ÿ��, 1�̸� �ؽ�Ʈ ���Ϸ�, �ٸ� ���̸� sqlite3 db�� ����
		wofstream logFile; // pc trace ����� �����ϱ� ���� ���� ��Ʈ��
		sqlite3* recordDB; // pc trace ����� �����ϱ� ���� sqlite3 DB pointer
		sqlite3* searchDB; // dll ������ �������� ���� sqlite3 DB pointer
		sqlite3_stmt* stmt; // db ����� ���� sql statement
		wstring functionName; // pc�� trace�� �Լ� �̸�
		wregex re; // dll �̸� ������ ���� ���Խ�
		wsmatch match; // dll �̸� ���� ����� �����ϱ� ���� ����
		string extractDllName(const std::wstring& dllPath);
		wstring getExecutablePath();
		wstring findClosestFunctionByRVA(sqlite3* db, DWORD rva, const std::string& tableName);
		wstring findDllNameByPc(PVOID pc);
		void record2Text(PVOID pc, wstring dllName, DWORD threadID);
		void record2DB(PVOID pc, wstring dllName, DWORD threadID);
	public:
		RECORD(tstring searchDBPath, int log_type);
		~RECORD();
		void log(atomic_bool* isDebuggerOn);
	};
}