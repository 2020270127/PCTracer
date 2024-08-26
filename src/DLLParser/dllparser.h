#pragma once

#include "typedef.h"
#include "peparser.h"
#include "peprint.h"
#include "strconv.h"

class dllParser
{

private : 
	peparser::PEParser PEParser_;
	peparser::PEPrint PEPrint_;
	logging::Logger Logger_;

	tstring globalDBName;
	void saveDLLEATToSQLTable(const tstring& dllPath, const tstring& sqlTableName);
	

public:
	dllParser(tstring DBName) : globalDBName(DBName) {};
	~dllParser() {};	
	void parseDLLEATRecursively(const tstring& directoryPath);
};

