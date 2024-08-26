#include "peprint.h"

namespace peparser
{
#define LINE_SPLIT _T("\n---------------------------------------------------------------------------------\n")

    void PEPrint::printEAT(const PE_STRUCT& peStructure)
    {
        if (!peStructure.exportFunctionList.empty())
        {
            for (auto const& element : peStructure.exportFunctionList)
            {
                logger_.log(format(_T("EAT Module: {:s} ({:d})"), get<0>(element), get<1>(element).size()));
                for (auto const& funcElement : get<1>(element))
                {
                    if (peStructure.is32Bit)
                    {
                        dbLogger_.printToConsoleAndLogToSQLDB(funcElement.Name, funcElement.Ordinal, funcElement.Address); 
                    }
                    else
                    {
                        dbLogger_.printToConsoleAndLogToSQLDB(funcElement.Name, funcElement.Ordinal, funcElement.Address); 
                    }
                }
                logger_.log(_T("\n"));
            }
            logger_.log(LINE_SPLIT);
        }
    };

    void PEPrint::createTable(const tstring& dbPath, const tstring& tableName)
    {
        dbLogger_.createTable(dbPath, tableName);
    }
};

