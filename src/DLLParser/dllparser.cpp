#include "dllparser.h"

using namespace peparser;
using namespace std;

void dllParser::saveDLLEATToSQLTable(const tstring& dllPath, const tstring& sqlTableName)
{
    if (PEParser_.open(dllPath))
    {
        PEPrint_.createTable(globalDBName, sqlTableName);
        if (PEParser_.parseEATCustom())
        {
            Logger_.log(format(_T("Parsing{} ...\n"), sqlTableName), LOG_LEVEL_ALL);
            PEPrint_.printEAT(PEParser_.getPEStructure());
        }
        else
        {
            Logger_.log(format(_T("Parsing {} failed \n"), sqlTableName), LOG_LEVEL_ERROR);
        }
    }
    else
    {
        Logger_.log(format(_T("Opening {} Failed\n"), dllPath), LOG_LEVEL_ERROR);
    }
    PEParser_.close();
}

void dllParser::parseDLLEATRecursively(const tstring& targetDirectoryPath)
{
    tstring targetDLLPath; 
    tstring sqlTableName;

    try
    {
        for (const auto& entry : filesystem::directory_iterator(targetDirectoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == _T(".dll"))
            {
                targetDLLPath = entry.path().string();
                sqlTableName = entry.path().stem().string(); 
                saveDLLEATToSQLTable(targetDLLPath, sqlTableName);
            }
        }
    }
    catch (const filesystem::filesystem_error& e)
    {
        Logger_.log(format(_T("Filesystem error: {}\n"), e.what()), LOG_LEVEL_ERROR);
    }
    catch (const exception& e)
    {
        Logger_.log(format(_T("General error: {}\n"), e.what()), LOG_LEVEL_ERROR);
    }
}


