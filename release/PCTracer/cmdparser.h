#pragma once

#include "typedef.h"
#include <stdexcept>

namespace cmdutil
{
    using namespace std;

    enum OPTION_INFO_TYPE
    {
        NAME = 0,
        ALTERNATIVE,
        DESCRIPTION
    };

    class CmdBase
    {
    protected:
        bool required_;         // 필수 옵션 여부
        bool isSet_;            // 옵션 값이 설정되었는 지 여부
        tstring name_;          // 옵션 이름
        tstring alternative_;   // 옵션 별칭
        tstring description_;   // 옵션 설명

    public:
        CmdBase(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            required_(required), isSet_(false), name_(name), alternative_(alternative), description_(description) {};
        virtual ~CmdBase(void) {};

        virtual bool compare(const tstring& name);
        virtual bool isRequired(void);
        virtual bool isSet(void);
        virtual tstring getOptionInfo(const OPTION_INFO_TYPE& type);
    };

    template<typename T>
    class CmdOption : public CmdBase
    {
    private:
        T value_;

    public:
        CmdOption(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            CmdBase(name, alternative, description, required) {};
        virtual ~CmdOption(void) {};

        // 템플릿 함수의 경우 헤더에 구현이 같이 포함되어야 함
        // hpp 파일을 이용해서 파일을 분리해서 작성할 수는 있지만 오히려 코드 가독성을 저하시킬 수 있음

        T get(void) { return value_; };
        void set(T value)
        {
            value_ = value;
            isSet_ = true;
        };
    };

    class CmdParser
    {
    private:
        bool help_;
        vector<CmdBase*> command_;

    public:
        CmdParser(void) : help_(false) {};
        virtual ~CmdParser(void) { clear(); };

        bool hasOptions(void);
        bool isRequiredOptionSet(void);
        bool isPrintHelp(void);
        void clear(void);
        vector<CmdBase*>::iterator find(const tstring& name);
        void parseCmdLine(int argc, TCHAR* argv[]);
        tstring getHelpMessage(const tstring& program);

        // 템플릿 함수의 경우 헤더에 구현이 같이 포함되어야 함
        // hpp 파일을 이용해서 파일을 분리해서 작성할 수는 있지만 오히려 코드 가독성을 저하시킬 수 있음

        // 필수 옵션 등록
        template<typename T>
        void set_required(const tstring& name, const tstring& alternative, const tstring& description = "")
        {
            command_.push_back(dynamic_cast <CmdBase*>(new CmdOption<T>(name, alternative, description, true)));
        };

        // 선택 옵션 등록
        template<typename T>
        void set_optional(const tstring& name, const tstring& alternative, T defaultValue, const tstring& description = "")
        {
            CmdOption<T>* cmd_option = new CmdOption<T>(name, alternative, description, false);
            cmd_option->set(defaultValue);

            command_.push_back(dynamic_cast <CmdBase*>(cmd_option));
        };

        // 옵션에 설정된 값 얻기
        template<typename T>
        T get(const tstring& name)
        {
            vector<CmdBase*>::iterator iter = find(name);
            if (iter != command_.end())
            {
                return (dynamic_cast <CmdOption<T> *>(*iter))->get();
            }
            else
            {
                throw runtime_error("The option does not exist!");
            }
        };
    };
};

