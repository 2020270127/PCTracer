#include "record.h"
#include "debugger.h"
#include "strconv.h"
#include <filesystem>

namespace record
{
    using namespace std;
    using namespace strconv;
    using namespace debugger;

    RECORD::RECORD(tstring searchDBPath, int log_type) : log_type_(log_type), recordDB(nullptr),
        functionName(L"Unknown Function"), re(LR"(([^\\]+)\.dll$)", std::regex_constants::icase)
    {
        if (sqlite3_open(tstringToString(searchDBPath).c_str(), &searchDB) != SQLITE_OK) {
            std::string errMsg = "Failed to open database: " + std::string(sqlite3_errmsg(searchDB));
            sqlite3_close(searchDB);
            throw std::runtime_error(errMsg);
        }

        wstring basePath = getExecutablePath();

        if (log_type == 1) // textLog
        {
            wstring logFilePath = basePath + L"\\" + L"logs.txt";
            std::filesystem::path path(logFilePath);
            logFile.open(path);
            if (!logFile) {
                wcerr << L"Failed to open log file: " << logFilePath << endl;
                throw runtime_error("Failed to open log file");
            }
        }
        else // database log
        {
            wstring dbPath = basePath + L"\\logs.db";
            if (sqlite3_open(string(dbPath.begin(), dbPath.end()).c_str(), &recordDB) != SQLITE_OK) {
                string errMsg = "Failed to open database: " + string(sqlite3_errmsg(recordDB));
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

    wstring RECORD::getExecutablePath()
    {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
        return wstring(buffer).substr(0, pos);
    }

    wstring RECORD::findClosestFunctionByRVA(sqlite3* db, DWORD rva, const string& tableName) {
        string sql("SELECT Name FROM " + tableName + " WHERE RVA <= ? ORDER BY RVA DESC LIMIT 1;");

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, rva);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                string utf8Name(name);
                functionName = utf8ToWstring(utf8Name);
            }
            sqlite3_finalize(stmt);
        }
        else {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        }

        return functionName;
    }

    string RECORD::extractDllName(const wstring& dllPath) {
        if (regex_search(dllPath, match, re) && match.size() > 1) {
            wstring wDllName = match.str(1);
            return wstringToUtf8(wDllName);
        }
        return "";
    }

    wstring RECORD::findDllNameByPc(PVOID pc) {
        DWORD rva;
        string dllName;
        wstring functionName;
        wstringstream ss;
        wchar_t buffer[256];

        for (const auto& dll : dllList) {
            if (pc >= dll.baseAddr && pc < (PVOID)((uintptr_t)dll.baseAddr + dll.size)) {
                ZeroMemory(buffer, sizeof(buffer));
                ss.clear();
                
                rva = (DWORD)((uintptr_t)pc - (uintptr_t)dll.baseAddr);
                dllName = extractDllName(dll.name);
                functionName = findClosestFunctionByRVA(searchDB, rva, dllName);  
                ss << utf8ToWstring(dllName) << L" -> 0x" << hex << rva << L", Function: " << functionName;

                return ss.str();
            }
        }
        return L"Unknown Module";
    }

    void RECORD::record2Text(PVOID pc, wstring dllName, DWORD threadID)
    {
        logFile << L"PC: " << pc << L" in " << dllName << L", Thread ID: " << threadID << endl;
    }

    void RECORD::record2DB(PVOID pc, wstring dllName, DWORD threadID) {
        if (!recordDB) {
            throw runtime_error("Database is not open");
        }

        sqlite3_stmt* stmt;
        const char* insertSQL = "INSERT INTO log (pc, dllName, threadID) VALUES (?, ?, ?);";
        if (sqlite3_prepare_v2(recordDB, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
            string errMsg = "Failed to prepare insert statement: " + string(sqlite3_errmsg(recordDB));
            throw runtime_error(errMsg);
        }

        // �����͸� 16���� ���ڿ��� ��ȯ
        stringstream ss;
        ss << "0x" << hex << (uintptr_t)pc;
        string pcStr = ss.str();

        // wstring�� string���� ��ȯ
        string dllNameStr(dllName.begin(), dllName.end());

        sqlite3_bind_text(stmt, 1, pcStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, dllNameStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, threadID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            string errMsg = "Failed to execute insert statement: " + string(sqlite3_errmsg(recordDB));
            sqlite3_finalize(stmt);
            throw runtime_error(errMsg);
        }

        sqlite3_finalize(stmt);
    }

    void RECORD::log(atomic_bool* isDebuggerOn) 
    {
        wstring dllName;
        size_t index = 0;
        PcInfo pcInfo;

        cout << "logging started..." << endl;
        do
        {
            while (!pcManager.isEmpty())
            {
                try {
                    pcInfo = pcManager.getPcInfo(); 
                }
                catch (const out_of_range& e) {
                    cerr << "Index out of range: " << e.what() << endl;
                    break;
                }

                dllName = findDllNameByPc(pcInfo.pc);

                if (log_type_ == 1)
                    record2Text(pcInfo.pc, dllName, pcInfo.threadId);
                else
                    record2DB(pcInfo.pc, dllName, pcInfo.threadId);
            }
        } while (*isDebuggerOn);

        cout << "logging finished!" << endl;
    }
}
