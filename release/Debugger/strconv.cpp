#include "strconv.h"
namespace strconv
{
    using namespace std;

    const string wstringToUtf8(const wstring& wstr) {
        if (wstr.empty()) {
            return std::string();
        }
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }

    // UTF8 string을 wide string으로 변환하는 함수, sqlite db에서 string을 가져오기 위해 사용
    wstring utf8ToWstring(const string& utf8Str) {
        if (utf8Str.empty()) return wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), NULL, 0);
        wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    LPWSTR tstringToLPWSTR(const tstring& tstr) {
#ifdef UNICODE
        // tstring이 std::wstring인 경우
        LPWSTR lpwstr = new wchar_t[tstr.size() + 1];
        wcscpy_s(lpwstr, tstr.size() + 1, tstr.c_str());
        return lpwstr;
#else
        // tstring이 std::string인 경우
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wstr = converter.from_bytes(tstr);
        LPWSTR lpwstr = new wchar_t[wstr.size() + 1];
        wcscpy_s(lpwstr, wstr.size() + 1, wstr.c_str());
        return lpwstr;
#endif
    }

    string tstringToString(const tstring& tstr) {
#ifdef UNICODE
        // tstring이 std::wstring인 경우
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(tstr);
#else
        // tstring이 std::string인 경우
        return tstr;
#endif
    }

    wstring tstringToWstring(const tstring& tstr) {
#ifdef UNICODE
        // tstring이 std::wstring인 경우
        return tstr;
#else
        // tstring이 std::string인 경우
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(tstr);
#endif
    }
}
