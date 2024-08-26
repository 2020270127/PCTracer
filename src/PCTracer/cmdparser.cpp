#include "cmdparser.h"

namespace cmdutil
{
    using namespace std;

    // �ɼ� �̸� �� (�̸� �Ǵ� ��Ī�� ��ġ�ϴ� �� ���� Ȯ��)
    bool CmdBase::compare(const tstring& name)
    {
        return ((name_.compare(name) == 0) || (alternative_.compare(name) == 0));
    };

    // �ʼ� �ɼ����� ���� Ȯ��
    bool CmdBase::isRequired(void)
    {
        return required_;
    };

    // �ɼ� ���� �����Ǿ��� �� ���� Ȯ��
    bool CmdBase::isSet(void)
    {
        return isSet_;
    };

    // OPTION_INFO_TYPE�� ���ǵ� �ɼ��� ������ ����
    tstring CmdBase::getOptionInfo(const OPTION_INFO_TYPE& type)
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

    // �ɼ��� �ϳ��� ��� �Ǿ����� ���� Ȯ��
    bool CmdParser::hasOptions(void)
    {
        return !command_.empty();
    };

    // ��ϵǾ� �ִ� �ʼ� �ɼǵ��� �� �����Ǿ����� üũ
    bool CmdParser::isRequiredOptionSet(void)
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

    // ���� ��� ���� Ȯ��
    // (���� ��� �ɼ��� �����Ǿ��ų� ��ϵ� �ɼ��� ���ų� 
    // �ʼ� �ɼ��� ���� �������� ���� ��� ���� ����� �ʿ�)
    bool CmdParser::isPrintHelp(void)
    {
        return (help_ || (!hasOptions()) || (!isRequiredOptionSet()));
    };

    // ����� �ɼ� ��� ����
    void CmdParser::clear(void)
    {
        for (const auto& iter : command_)
        {
            delete iter;
        }
        command_.clear();
        help_ = false;
    };

    // �̸����� �ɼ��� ����� ��ġ�� iterator ã��
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

    // command line parsing
    void CmdParser::parseCmdLine(int argc, TCHAR* argv[])
    {
        tstring name;
        tstring command_str;
        int char_index = 0;

        if (argc < 1)
        {
            printf("No arguments provided.\n");
        }
        else
        {
            // ù ��° �׸��� ���� ���� �ڽ��� �̸��̱� ������ ����
            for (int index = 1; index < argc; index++)
            {
                // ���� ��� �ɼ��� ���� �ƴ��� Ȯ�� 
                name = argv[index];
                if ((name.compare(_T("-h")) == 0) || (name.compare(_T("--help")) == 0))
                {
                    help_ = true;
                    break;
                }
                else if (name.at(0) == '-')
                {
                    // �ɼ� ���ڿ����� '-' ���� ����
                    name.erase(remove(name.begin(), name.end(), '-'), name.end());

                    // ��ϵ� �ɼ����� ���θ� Ȯ���Ͽ�, ��ϵ� �ɼ��̸� �Էµ� ���� �о ����
                    // iter == vector<CmdBase*>::iterator
                    const auto& iter = find(name);

                    if ((iter != command_.end()) && (++index < argc)) // ++index�� �ɼǿ� ������ �� ��ġ�� �̵�
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
                            // typeid �ν� ������ Ÿ�Կ� ���ؼ� �߰� ����
                            else
                            {
                                // typeid name()�� char*
                                cout << format("Unknown option type : {}\n", typeid(*(*iter)).name());
                            }
                        }
                        catch (const exception& e)
                        {
                            cout << "Error occuerd : "  << e.what() << endl;
                        }
                    }
                }
            }
        }
    };


    tstring CmdParser::getHelpMessage(const tstring& program)
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
