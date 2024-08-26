#pragma once

#include "typedef.h"
#include <stdexcept>

namespace cmdparser
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
        bool required_;      
        bool isSet_;         
        tstring name_;        
        tstring alternative_;  
        tstring description_;  

    public:
        CmdBase(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            required_(required), isSet_(false), name_(name), alternative_(alternative), description_(description) {};
        virtual ~CmdBase() = default;
        virtual bool compare(const tstring& name) const;
        virtual bool isRequired(void) const;
        virtual bool isSet(void) const;
        virtual tstring getOptionInfo(const OPTION_INFO_TYPE& type) const;
    };


    template<typename T,
        typename = typename std::enable_if_t< 
        std::is_same<T, int>::value || 
        std::is_same<T, float>::value || 
        std::is_same<T, tstring>::value>>
    class CmdOption : public CmdBase
    {
    private:
        T value_;

    public:
        CmdOption(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            CmdBase(name, alternative, description, required) {};
        virtual ~CmdOption() override = default;

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

    private:
        bool hasOptions(void) const;
        bool isRequiredOptionSet(void) const;
        vector<CmdBase*>::iterator find(const tstring& name);

    public:
        CmdParser() : help_(false) {};
        ~CmdParser() { clear(); };
        void clear(void);
        bool isPrintHelp(void) const;
        void parseCmdLine(int argc, TCHAR* argv[]);
        tstring getHelpMessage(const tstring& program) const;

       
        template<typename T>
        void set_required(const tstring& name, const tstring& alternative, const tstring& description = "")
        {
            command_.push_back(dynamic_cast <CmdBase*>(new CmdOption<T>(name, alternative, description, true)));
        };

        
        template<typename T>
        void set_optional(const tstring& name, const tstring& alternative, T defaultValue, const tstring& description = "")
        {
            CmdOption<T>* cmd_option = new CmdOption<T>(name, alternative, description, false);
            cmd_option->set(defaultValue);

            command_.push_back(dynamic_cast <CmdBase*>(cmd_option));
        };

  
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

