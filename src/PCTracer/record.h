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
		tofstream logFile; 
		sqlite3* recordDB;
		sqlite3* searchDB;
		sqlite3_stmt* stmt; 
		tregex re;
		tstring functionName;
		tsmatch match;

	private:
		tstring extractDllName(const tstring& dllPath);
		tstring getExecutablePath();
		tstring findClosestFunctionByRVA(sqlite3* db, DWORD rva, const std::string& tableName);
		tstring findDLLBelongingNameByPc(PVOID pc);
		 void record2Text(PVOID pc, tstring dllName, DWORD threadID);
		void record2DB(PVOID pc, tstring dllName, DWORD threadID);

	private:
		strconv::StrConv StrConv_;
		logging::Logger Logger_;

	public:
		RECORD(tstring searchDBPath, int log_type);
		~RECORD();
		void logUntilPcmanagerIsEmpty(atomic_bool* isDebuggerOn);
	};
}