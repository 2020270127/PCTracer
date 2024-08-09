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
		DatabaseLogger dbLogger_;

	public:
		//PEPrint();
		PEPrint(const string& tableName);
		~PEPrint() = default;
		void printEAT(const PE_STRUCT& peStructure);
	};
};

