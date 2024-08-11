#pragma once
#include <locale>
#include <codecvt>
#include "typedef.h"

namespace strconv
{
	using namespace std;

	const string  wstringToUtf8(const std::wstring& wstr);
	wstring  utf8ToWstring(const string& utf8Str);
	LPWSTR tstringToLPWSTR(const tstring& tstr);
	string tstringToString(const tstring& tstr);
	wstring tstringToWstring(const tstring& tstr);
}