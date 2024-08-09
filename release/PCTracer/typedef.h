#pragma once
#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <queue>
#include <map>
#include <tchar.h>
#include <typeinfo>
#include <string_view>
#include <algorithm>
#include <windows.h>
#include <psapi.h>
#include <regex>
#include <locale>
#include <codecvt>
#include <stdio.h>
#include <unordered_map>
#include <utility>
#include <tlhelp32.h>
#include <cstdint>
#include <fstream>
#include <sstream>
#include "sqlite3.h"

#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#else
#define tcout cout
#endif

typedef std::basic_string<TCHAR> tstring;