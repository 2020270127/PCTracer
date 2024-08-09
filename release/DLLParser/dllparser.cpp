#include "dllparser.h"



using namespace peparser;
using namespace std;



void dllParser::processDLL(const wstring& dllPath, const string& tableName)
{
    PEParser* peParser = new PEParser();
    if (peParser->open(dllPath.c_str()))
    {
        PEPrint* pePrint = new PEPrint(tableName);
        if (peParser->parseEATCustom())
        {
            cout << "Parsing " << tableName << "..." << endl;
            pePrint->printEAT(peParser->getPEStructure());
        }
        else
        {
            cout << "Parsing " << tableName << " failed" << endl;
        }
        delete pePrint;
    }
    else
    {
        wcout << "Opening " << dllPath << " Failed" << endl;
    }
    delete peParser;
}

void dllParser::ParseDllRecursively(const wstring& directoryPath)
{
    wstring filePath; // dll 경로
    string fileName; // sql 테이블에 기입할 dll 이름

    try
    {
        for (const auto& entry : filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == L".dll")
            {
                filePath = entry.path().wstring();
                fileName = entry.path().stem().string(); // 파일명에서 확장자를 제거한 이름을 추출
                processDLL(filePath, fileName);
            }
        }
    }
    catch (const filesystem::filesystem_error& e)
    {
        wcerr << L"Filesystem error: " << e.what() << endl;
    }
    catch (const exception& e)
    {
        wcerr << L"General error: " << e.what() << endl;
    }
}


