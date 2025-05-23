#pragma once

#include <windows.h>
#include <psapi.h>
#include <regex>
#include <iostream>
#include <tchar.h>
#include <format>
#include <fstream>
#include "sqlite3.h"

typedef std::basic_string<TCHAR> tstring;

#if defined(UNICODE) || defined(_UNICODE)
	#define tcout wcout
	typedef std::wregex tregex;
	typedef std::wsmatch tsmatch;
	typedef std::wofstream tofstream;
#else
	#define tcout cout
	typedef std::regex tregex;
	typedef std::match_results<tstring::const_iterator> tsmatch;
	typedef std::ofstream tofstream;
#endif



