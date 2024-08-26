#pragma once
#include <atomic>
#include <regex>
#include "typedef.h"
#include "logger.h"
#include "strconv.h"

namespace record
{
	using namespace std;

	class RECORD
	{
	private:
		int log_type_; 
		 // wofstream logFile; 
		sqlite3* recordDB;
		sqlite3* searchDB;
		sqlite3_stmt* stmt; 
		tregex re;

		tstring functionName;
		
		tsmatch match;
		tstring extractDllName(const tstring& dllPath);

		tstring getExecutablePath();
		tstring findClosestFunctionByRVA(sqlite3* db, DWORD rva, const std::string& tableName);
		tstring findDllNameByPc(PVOID pc);

		// void record2Text(PVOID pc, tstring dllName, DWORD threadID);
		void record2DB(PVOID pc, tstring dllName, DWORD threadID);
		strconv::StrConv StrConv_;
		logging::Logger Logger_;
	public:
		RECORD(tstring searchDBPath, int log_type);
		~RECORD();
		void log(atomic_bool* isDebuggerOn);
	};
}