#pragma once

#include "typedef.h"
#include "sqlite3.h"
#include "strconv.h"
#include <iomanip>
#include <sstream>


namespace logging
{
	using namespace std;
	using namespace strconv;

	enum LogLevel
	{
		LOG_LEVEL_OFF = 0,
		LOG_LEVEL_ALL,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_FATAL
	};

	enum LogDirection
	{
		LOG_DIRECTION_DEBUGVIEW = 0,
		LOG_DIRECTION_CONSOLE
	};

	class Logger
	{
	private:
		bool addFuncInfo_;
		LogLevel logLevel_;
		LogDirection logDirection_;

	private:
		void output(const string_view& logMessage, const bool& useEndl = true) const;
		void output(const wstring_view& logMessage, const bool& useEndl = true) const;

	public:
		Logger(bool addFuncInfo = false, LogLevel logLevel = LOG_LEVEL_ALL, LogDirection logDirection = LOG_DIRECTION_CONSOLE) :
			addFuncInfo_(addFuncInfo), logLevel_(logLevel), logDirection_(logDirection) {};
		~Logger() {};
		void setLogType(const LogLevel& logLevel, const LogDirection& logDirection, const bool& addFuncInfo = true);
		void getLogType(LogLevel& logLevel, LogDirection& logDirection, bool& addFuncInfo) const;
		void log(const string_view& logMessage, const LogLevel& logLevel = LOG_LEVEL_ALL, const bool& useEndl = true, const char* funcName = __builtin_FUNCTION(), int funcLine = __builtin_LINE()) const;
		void log(const wstring_view& logMessage, const LogLevel& logLevel = LOG_LEVEL_ALL, const bool& useEndl = true, const char* funcName = __builtin_FUNCTION(), int funcLine = __builtin_LINE()) const; 
		void log(const string_view& logMessage, const uint32_t& errorCode, const LogLevel& logLevel = LOG_LEVEL_ALL, const bool& useEndl = true, const char* funcName = __builtin_FUNCTION(), int funcLine = __builtin_LINE()) const;
		void log(const wstring_view& logMessage, const uint32_t& errorCode, const LogLevel& logLevel = LOG_LEVEL_ALL, const bool& useEndl = true, const char* funcName = __builtin_FUNCTION(), int funcLine = __builtin_LINE()) const;
	};

	class SQLDBLogger
	{
	private:
		StrConv StrConv_;
		Logger Logger_;
		sqlite3* globalSQLDBPointer;             
		tstring globalTSQLTableName; 
		
	private:
		void LogToSQLDB(tstring functionName, size_t functionOrdinal, size_t functionAddress) ;

	public:
		void printToConsoleAndLogToSQLDB(tstring functionName, size_t functionOrdinal, size_t functionAddress);
		void createTable(const tstring& sqlDBPath, const tstring& sqlTableName);

	public:
		SQLDBLogger(const tstring& sqlDBPath, const tstring& sqlTableName);
		~SQLDBLogger();
	};
};

