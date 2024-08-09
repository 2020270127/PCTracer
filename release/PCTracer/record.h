#pragma once
#include "typedef.h"

namespace record
{
	using namespace std;

	class RECORD
	{
	private:
		int log_type_; // 로깅 타입, 1이면 텍스트 파일로, 다른 값이면 sqlite3 db로 저장
		wofstream logFile; // pc trace 기록을 저장하기 위한 파일 스트림
		sqlite3* recordDB; // pc trace 기록을 저장하기 위한 sqlite3 DB pointer
		sqlite3* searchDB; // dll 정보를 가져오기 위한 sqlite3 DB pointer
		sqlite3_stmt* stmt; // db 기록을 위한 sql statement
		wstring functionName; // pc로 trace한 함수 이름
		wregex re; // dll 이름 추출을 위한 정규식
		wsmatch match; // dll 이름 추출 결과를 저장하기 위한 변수
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