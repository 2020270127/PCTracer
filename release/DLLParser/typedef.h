#pragma once

#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <tuple>
#include <tchar.h>
#include <typeinfo>
#include <filesystem>
#include <string_view>
#include <algorithm>
#include "windows.h"



#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#define to_string_t to_wstring
#else
#define tcout cout
#define to_string_w to_string
#endif

// TCHAR를 다룰 문자열 형식 정의
typedef std::basic_string<TCHAR> tstring;
typedef std::basic_string_view<TCHAR> tstring_view;

// 바이너리 데이터 저장을 위한 형식 정의
typedef std::vector<BYTE> BinaryData;

