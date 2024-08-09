#pragma once

#include "peprint.h"

namespace peparser
{
#define LINE_SPLIT _T("\n---------------------------------------------------------------------------------\n")

    PEPrint::PEPrint(const string& tableName) : dbLogger_(L"DLL.db", tableName)
    {
        logger_.setLogType(LOG_LEVEL_ALL, LOG_DIRECTION_CONSOLE, FALSE);
    };

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
                        dbLogger_.log(funcElement.Name, funcElement.Ordinal, funcElement.Address); // Name, Ordianl, RVA 순으로 기입
                    }
                    else
                    {
                        dbLogger_.log(funcElement.Name, funcElement.Ordinal, funcElement.Address); // Name, Ordianl, RVA 순으로 기입
                    }
                }
                logger_.log(_T("\n"));
            }
            logger_.log(LINE_SPLIT);
        }
    };
};

