#include "logger.h"
#include "strconv.h"

namespace logging
{
    using namespace std;
    using namespace strconv;

    void Logger::setLogType(const LogLevel& logLevel, const LogDirection& logDirection, const bool& addFuncInfo)
    {
        logLevel_ = logLevel;
        logDirection_ = logDirection;
        addFuncInfo_ = addFuncInfo;
    };

    void Logger::getLogType(LogLevel& logLevel, LogDirection& logDirection, bool& addFuncInfo) const
    {
        logLevel = logLevel_;
        logDirection = logDirection_;
        addFuncInfo = addFuncInfo_;
    };

    void Logger::output(const string_view& logMessage, const bool& useEndl) const
    {
        if (logLevel_ > LOG_LEVEL_OFF)
        {
            if (logDirection_ == LOG_DIRECTION_DEBUGVIEW)
            {
                OutputDebugStringA(logMessage.data());
                if (useEndl)
                {
                    OutputDebugStringA("\n");
                }
            }
            else if (logDirection_ == LOG_DIRECTION_CONSOLE)
            {
                cout << logMessage.data();
                if (useEndl)
                {
                    cout << endl;
                }
            }
        }
    };

    void Logger::output(const wstring_view& logMessage, const bool& useEndl) const
    {
        if (logLevel_ > LOG_LEVEL_OFF)
        {
            if (logDirection_ == LOG_DIRECTION_DEBUGVIEW)
            {
                OutputDebugStringW(logMessage.data());
                if (useEndl)
                {
                    OutputDebugStringW(L"\n");
                }
            }
            else if (logDirection_ == LOG_DIRECTION_CONSOLE)
            {
                wcout << logMessage;
                if (useEndl)
                {
                    wcout << endl;
                }
            }
        }
    };

    void Logger::log(const string_view& logMessage, const LogLevel& logLevel, const bool& useEndl, const char* funcName, int funcLine) const
    {
        if (logLevel >= logLevel_)
        {
            if (addFuncInfo_)
            {
                output(format("Function = {:s}({:d}), Msg = ", funcName, funcLine), false);
            }
            output(format("{:s}", logMessage), useEndl);
        }
    };

    void Logger::log(const wstring_view& logMessage, const LogLevel& logLevel, const bool& useEndl, const char* funcName, int funcLine) const
    {
        if (logLevel >= logLevel_)
        {
            if (addFuncInfo_)
            {
                output(format("Function = {:s}({:d}), Msg = ", funcName, funcLine), false);
            }
            output(format(L"{:s}", logMessage), useEndl);
        }
    };

    void Logger::log(const string_view& logMessage, const uint32_t& errorCode, const LogLevel& logLevel, const bool& useEndl, const char* funcName, int funcLine) const
    {
        if (logLevel >= logLevel_)
        {
            if (addFuncInfo_)
            {
                output(format("Function = {0:s}({1:d}), Error code = 0x{2:x}({2:d}), Msg = ", funcName, funcLine, errorCode), false);
            }
            output(format("{:s}", logMessage), useEndl);
        }
    };

    void Logger::log(const wstring_view& logMessage, const uint32_t& errorCode, const LogLevel& logLevel, const bool& useEndl, const char* funcName, int funcLine) const
    {
        if (logLevel >= logLevel_)
        {
            if (addFuncInfo_)
            {
                output(format("Function = {0:s}({1:d}), Error code = 0x{2:x}({2:d}), Msg = ", funcName, funcLine, errorCode), false);
            }
            output(format(L"{:s}", logMessage), useEndl);
        }
    };

    
    SQLDBLogger::SQLDBLogger(const tstring& tSQLDBPath, const tstring& tableName) 
    {
        if (sqlite3_open(tSQLDBPath.c_str(), &globalSQLDBPointer))
        {
            Logger_.log(format(_T("Can't open database: {}\n"), sqlite3_errmsg(globalSQLDBPointer)),LOG_LEVEL_ERROR);
            globalSQLDBPointer = nullptr;
        }         
    }

    void SQLDBLogger::createTable(const tstring& tSQLDBPath, const tstring& sqlTableName) 
    {
        globalTSQLTableName = sqlTableName;

        if (sqlite3_open(tSQLDBPath.c_str(), &globalSQLDBPointer))
        {
            Logger_.log(format(_T("Can't open database: {}"), sqlite3_errmsg(globalSQLDBPointer)), LOG_LEVEL_ERROR);
            globalSQLDBPointer = nullptr;
        }
        else
        {
            string sql = "CREATE TABLE IF NOT EXISTS " + globalTSQLTableName + " ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "RVA TEXT,"
                "Ordinal TEXT,"
                "Name TEXT);";
            char* errMsg = nullptr;
            if (sqlite3_exec(globalSQLDBPointer, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK)
            {
                Logger_.log(format(_T("SQL error: {}"), errMsg), LOG_LEVEL_ERROR);
                wcout << L"SQL error: " << errMsg << endl;
                sqlite3_free(errMsg);
            }
        }
    }
    
    SQLDBLogger::~SQLDBLogger()
    {
        if (globalSQLDBPointer)
        {
            sqlite3_close(globalSQLDBPointer);
        }
    }
    
    void SQLDBLogger::logToSQLDB(tstring functionName, size_t functionOrdinal, size_t functionAddress) 
    {
        if (globalSQLDBPointer)
        {
            string sql = "INSERT INTO " + globalTSQLTableName + " (Name, Ordinal, RVA) VALUES (?, ?, ?);";
            sqlite3_stmt* stmt = nullptr;
            tstring Name = "No Information";
            if (sqlite3_prepare_v2(globalSQLDBPointer, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                if (functionName != "")
                {
                    Name = functionName;
                } 

                sqlite3_bind_text(stmt, 1, Name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, static_cast<int>(functionOrdinal));
                sqlite3_bind_int(stmt, 3, static_cast<int>(functionAddress));

                if (sqlite3_step(stmt) != SQLITE_DONE)
                {
                    Logger_.log(format(_T("SQL error:  {}\n"), sqlite3_errmsg(globalSQLDBPointer)), LOG_LEVEL_ERROR);
                }
                sqlite3_finalize(stmt);
            }
            else
            {
                Logger_.log(format(_T("SQL error:  {}\n"), sqlite3_errmsg(globalSQLDBPointer)), LOG_LEVEL_ERROR);
            }
        }
    }

   
    void SQLDBLogger::printToConsoleAndLogToSQLDB(tstring functionName, size_t functionOrdinal, size_t functionAddress)
    {
        Logger_.log(format(_T("Function {}({}), at {:x}"), functionName, functionOrdinal, functionAddress));
        logToSQLDB(functionName, functionOrdinal, functionAddress);
    }
};

