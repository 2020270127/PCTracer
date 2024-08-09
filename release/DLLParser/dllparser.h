#pragma once

#include "typedef.h"
#include "peparser.h"
#include "peprint.h"

class dllParser
{
public:
	dllParser() {};
	~dllParser() {};
	void processDLL(const std::wstring& dllPath, const std::string& tableName);
	void ParseDllRecursively(const std::wstring& directoryPath);
};

