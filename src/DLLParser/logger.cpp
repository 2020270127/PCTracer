#pragma once

#include "logger.h"

namespace logging
{
    using namespace std;

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
                output(format("Function = {:s}({:d}), Error code = 0x{2:x}({2:d}), Msg = ", funcName, funcLine, errorCode), false);
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

    // DatabaseLogger 생성자
    DatabaseLogger::DatabaseLogger(const wstring& dbPath, const string& tableName) : tableName_(tableName), Ordinal(0), Address(0)
    {
        if (sqlite3_open16(dbPath.c_str(), &db))
        {
            wcout << L"Can't open database: " << sqlite3_errmsg(db) << endl;
            db = nullptr;
        }
        else
        {
            string sql = "CREATE TABLE IF NOT EXISTS " + tableName_ + " ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "RVA TEXT,"
                "Ordinal TEXT,"
                "Name TEXT);";
            char* errMsg = nullptr;
            if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK)
            {
                wcout << L"SQL error: " << errMsg << endl;
                sqlite3_free(errMsg);
            }
        }
    }

    // DatabaseLogger 소멸자
    DatabaseLogger::~DatabaseLogger()
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }

    // 로그 메시지를 콘솔에 출력
    void DatabaseLogger::logToConsole(tstring Name, size_t Ordinal, size_t Address) const
    {
        wostringstream oss;
        oss << L"          > " << setw(18) << setfill(L'0') << hex << Address
            << L", " << setw(6) << setfill(L'0') << hex << Ordinal
            << L", " << Name;
        wcout << oss.str() << endl;
    }

    // 로그 메시지를 데이터베이스에 기록
    void DatabaseLogger::logToDatabase(tstring Name, size_t Ordinal, size_t Address) const
    {
        if (db)
        {
            string sql = "INSERT INTO " + tableName_ + " (Name, Ordinal, RVA) VALUES (?, ?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                ostringstream ordinalStream;
                ordinalStream << setw(6) << setfill('0') << hex << Ordinal;
                string ordinal = ordinalStream.str();

                sqlite3_bind_text16(stmt, 1, Name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, ordinal.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, static_cast<int>(Address));

                if (sqlite3_step(stmt) != SQLITE_DONE)
                {
                    wcout << L"SQL error: " << sqlite3_errmsg(db) << endl;
                }
                sqlite3_finalize(stmt);
            }
            else
            {
                wcout << L"SQL error: " << sqlite3_errmsg(db) << endl;
            }
        }
    }

    // 로그 메시지를 콘솔과 데이터베이스에 기록
    void DatabaseLogger::log(tstring Name, size_t Ordinal, size_t Address)
    {
        // logToConsole(Name, Ordinal, Address);
        logToDatabase(Name, Ordinal, Address);
    }
};

