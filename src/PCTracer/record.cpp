#include "record.h"
#include "debugger.h"
#include "strconv.h"
#include "logger.h"
#include "typedef.h"
#include <filesystem>

namespace record
{
    using namespace std;
    using namespace strconv;
    using namespace debugger;
    using namespace logging;

    RECORD::RECORD(tstring tSearchDBPath, int log_type) : log_type_(log_type), recordDB(nullptr),
        functionName("Unknown Function"), re(R"(([^\\]+)\.dll$)", std::regex_constants::icase)
    {
        if (sqlite3_open((tSearchDBPath).c_str(), &searchDB) != SQLITE_OK) {
            std::string errMsg = "Failed to open database: " + std::string(sqlite3_errmsg(searchDB));
            sqlite3_close(searchDB);
            throw std::runtime_error(errMsg);
        }

        tstring basePath = getExecutablePath();

        if (log_type == 1) // textLog
        {
            tstring logFilePath = basePath + "\\" + "logs.txt";
            std::filesystem::path path(logFilePath);
            logFile.open(path);
            if (!logFile) {
                Logger_.log(format(_T("Failed to open log file: {}\n"), logFilePath), LOG_LEVEL_ERROR);
                throw runtime_error("Failed to open log file");
            }
        }
        else // database log
        {
            tstring dbPath = basePath + "\\logs.db";
            if (sqlite3_open(dbPath.c_str(), &recordDB) != SQLITE_OK) {
                tstring errMsg = "Failed to open database: " + tstring(sqlite3_errmsg(recordDB));
                throw runtime_error(errMsg);
            }

            const char* createTableSQL =
                "CREATE TABLE IF NOT EXISTS log ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "pc TEXT, "
                "dllName TEXT, "
                "threadID INTEGER);";

            char* errMsg = nullptr;
            if (sqlite3_exec(recordDB, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
                string errMsgStr = "Failed to create table: " + string(errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(recordDB);
                throw runtime_error(errMsgStr);
            }
        }
    }

    RECORD::~RECORD()
    {
        if (recordDB) {
            sqlite3_close(recordDB);
        }
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    tstring RECORD::getExecutablePath()
    {
        tstring buffer(MAX_PATH, TEXT('\0'));  
        GetModuleFileName(NULL, &buffer[0], MAX_PATH); 
        buffer.resize(_tcslen(buffer.c_str()));

        tstring::size_type pos = buffer.find_last_of(TEXT("\\/"));
        return buffer.substr(0, pos);
    }

    tstring RECORD::findClosestFunctionByRVA(sqlite3* db, DWORD rva, const string& tableName) {
        tstring sql("SELECT Name FROM " + tableName + " WHERE RVA <= ? ORDER BY RVA DESC LIMIT 1;");
        tstring tName;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) 
        {
            sqlite3_bind_int(stmt, 1, rva);
            if (sqlite3_step(stmt) == SQLITE_ROW) 
            {
#ifdef UNICODE
                tName = StrConv_.ansi2unicode(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
#else
                tName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
#endif
            }
            sqlite3_finalize(stmt);
        }
        else {
            Logger_.log(format(_T("Failed to prepare statement: {}"), sqlite3_errmsg(db)));
        }
        return tName;
    }

    tstring RECORD::extractDllName(const tstring& dllPath) {

        if (regex_search(dllPath, match, re) && match.size() > 1) {
            tstring dllName = match.str(1).c_str();
            return dllName;
        }
        return "";
    }

    tstring RECORD::findDLLBelongingNameByPc(PVOID pc) {
        DWORD rva;
        tstring dllName;
        tstring functionName;
       
        tstring buffer;

        for (const auto& dll : LoadedDLLInfoList) {
            if (pc >= dll.baseAddr && pc < (PVOID)((uintptr_t)dll.baseAddr + dll.size)) {
                rva = (DWORD)((uintptr_t)pc - (uintptr_t)dll.baseAddr);
                dllName = extractDllName(dll.name);
                functionName = findClosestFunctionByRVA(searchDB, rva, dllName);  
                buffer = format(_T("{} -> 0x{:x}, Function {}"), dllName, rva, functionName);               

                return buffer;
            }
        }
        return "Unknown Module";
    }

    void RECORD::record2Text(PVOID pc, tstring dllName, DWORD threadID)
    {
        logFile << format(_T("PC: {} in {}, Thread ID: {}\n"), pc, dllName, threadID);
    }

    void RECORD::record2DB(PVOID pc, tstring dllName, DWORD threadID) {
        if (!recordDB) {
            throw runtime_error("Database is not open");
        }

        //sqlite3_stmt* stmt;
        tstring pcStr = format(_T("{:x}"), (uintptr_t)pc);
        const char* insertSQL = "INSERT INTO log (pc, dllName, threadID) VALUES (?, ?, ?);";

        if (sqlite3_prepare_v2(recordDB, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
            string errMsg = "Failed to prepare insert statement: " + string(sqlite3_errmsg(recordDB));
            throw runtime_error(errMsg);
        }

        sqlite3_bind_text(stmt, 1, pcStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, dllName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, threadID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            string errMsg = "Failed to execute insert statement: " + string(sqlite3_errmsg(recordDB));
            sqlite3_finalize(stmt);
            throw runtime_error(errMsg);
        }
        sqlite3_finalize(stmt);
    }

    void RECORD::logUntilPcmanagerIsEmpty(atomic_bool* isDebuggerOn) 
    {
        tstring dllName;
        size_t index = 0;
        PcInfo pcInfo;

        Logger_.log(_T("logging started...\n"));
        do
        {
            while (!pcManager.isEmpty())
            {
                try {
                    pcInfo = pcManager.getPcInfo(); 
                }
                catch (const out_of_range& e) {
                    Logger_.log(format(_T("Index out of range {}\n"), e.what()), LOG_LEVEL_ERROR);
                    break;
                }

                dllName = findDLLBelongingNameByPc(pcInfo.pc);

                if (log_type_ == 1)
                     record2Text(pcInfo.pc, dllName, pcInfo.threadId);
                 else
                    record2DB(pcInfo.pc, dllName, pcInfo.threadId);
            }
        } while (*isDebuggerOn);
        Logger_.log(_T("logging finished!\n"));
    }
}
