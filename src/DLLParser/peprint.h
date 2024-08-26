#pragma once

#include "typedef.h"
#include "ipebase.h"
#include "logger.h"

namespace peparser
{
	using namespace logging;

	class PEPrint {

	private:
		Logger logger_;
		SQLDBLogger dbLogger_;

	public:
		PEPrint() : dbLogger_("DLL.db", "defaultTable") {};
		~PEPrint() = default;
		void printEAT(const PE_STRUCT& peStructure);
		void createTable(const tstring& dbPath, const tstring& tableName);
	};
};

