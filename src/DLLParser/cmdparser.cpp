#include "cmdparser.h"

namespace cmdparser
{
    using namespace std;

    bool CmdBase::compare(const tstring& name) const
    {
        return ((name_.compare(name) == 0) || (alternative_.compare(name) == 0));
    };

    bool CmdBase::isRequired(void) const
    {
        return required_;
    };

    bool CmdBase::isSet(void) const
    {
        return isSet_;
    };

    tstring CmdBase::getOptionInfo(const OPTION_INFO_TYPE& type) const
    {
        switch (type)
        {
        case NAME:
            return name_;
        case ALTERNATIVE:
            return alternative_;
        case DESCRIPTION:
            return description_;
        default:
            return _T("");
        }
    };

    bool CmdParser::hasOptions(void) const
    {
        return !command_.empty();
    };

    bool CmdParser::isRequiredOptionSet(void) const
    {
        bool isSet = true;

        for (const auto& iter : command_)
        {
            if (!iter->isSet())
            {
                isSet = false;
                break;
            };
        }
        return isSet;
    }

    vector<CmdBase*>::iterator CmdParser::find(const tstring& name)
    {
        for (vector<CmdBase*>::iterator iter = command_.begin(); iter != command_.end(); iter++)
        {
            if ((*iter)->compare(name))
            {
                return iter;
            }
        }
        return command_.end();
    };

    void CmdParser::clear(void)
    {
        for (const auto& iter : command_)
        {
            delete iter;
        }
        command_.clear();
        help_ = false;
    };

    bool CmdParser::isPrintHelp(void) const
    {
        return (help_ || (!hasOptions()) || (!isRequiredOptionSet()));
    };

    // command line parsing
    void CmdParser::parseCmdLine(int argc, TCHAR* argv[])
    {
        tstring name;
        tstring command_str;
        int char_index = 0;

        if (argc < 1)
        {
            tcout << _T("No arguments provided.\n");
        }
        else
        {
            for (int index = 1; index < argc; index++)
            {
                name = argv[index];
                if ((name.compare(_T("-h")) == 0) || (name.compare(_T("--help")) == 0))
                {
                    help_ = true;
                    break;
                }
                else if (name.at(0) == '-')
                {
                    name.erase(remove(name.begin(), name.end(), '-'), name.end());

                    // iter == vector<CmdBase*>::iterator
                    const auto& iter = find(name);

                    if ((iter != command_.end()) && (++index < argc)) 
                    {
                        const auto& type = typeid(*(*iter));
                        try
                        {
                            if (type == typeid(CmdOption<int>))
                            {
                                (dynamic_cast<CmdOption<int> *>(*iter))->set(stoi(argv[index]));
                            }
                            else if (type == typeid(CmdOption<float>))
                            {
                                (dynamic_cast<CmdOption<float> *>(*iter))->set(stof(argv[index]));
                            }
                            else if (type == typeid(CmdOption<tstring>))
                            {
                                (dynamic_cast<CmdOption<tstring> *>(*iter))->set(tstring(argv[index]));
                            }
                            else
                            {
                                cout << format("Unknown option type : {}\n", typeid(*(*iter)).name());
                            }
                        }
                        catch (const exception& e)
                        {
                            cout << "Error occuerd : " << e.what() << endl;
                        }
                    }
                }
            }
        }
    };


    tstring CmdParser::getHelpMessage(const tstring& program) const
    {
        tstring help_msg;
        tstring required_option;
        tstring optional_option;

        for (auto& iter : command_)
        {
            if (iter->isRequired())
            {
                required_option.append(format(_T("-{}, --{} \t{}\n"),
                    iter->getOptionInfo(NAME),
                    iter->getOptionInfo(ALTERNATIVE),
                    iter->getOptionInfo(DESCRIPTION)));
            }
            else
            {
                optional_option.append(format(_T("-{}, --{}\t{}\n"),
                    iter->getOptionInfo(NAME),
                    iter->getOptionInfo(ALTERNATIVE),
                    iter->getOptionInfo(DESCRIPTION)));
            }
        }
        help_msg = format(_T("Usage: {} [options]\n\n"), program);
        if (required_option.length() > 0)
        {
            help_msg.append(format(_T("Required Otions:\n{}\n"), required_option));
        }
        if (optional_option.length() > 0)
        {
            help_msg.append(format(_T("Optional Otions:\n{}\n"), optional_option));
        }
        return help_msg;
    };
};
