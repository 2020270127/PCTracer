#pragma once

#include <string>
#include <windows.h>
#include <psapi.h>


#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#else
#define tcout cout
#endif

typedef std::basic_string<TCHAR> tstring;