#include "debugger.h"
#include "record.h"
#include "cmdparser.h"
#include "typedef.h"
#include <thread>

using namespace cmdutil;
using namespace debugger;
using namespace record;

shared_ptr<Debug> sharedDebugger; 

void configure_parser(CmdParser& parser)
{
	parser.set_required<tstring>(_T("t"), _T("target_path"), _T("The target's path"));
	parser.set_required<tstring>(_T("d"), _T("db_path"), _T("The DB's path"));
	parser.set_optional<int>(_T("l"), _T("log_level"), 1, _T("log_level: TEXT for 1, DB for 2"));
};

void print_hep_message(CmdParser& parser)
{
	tcout << parser.getHelpMessage(_T("PC_TRACER"));
};

int _tmain(int argc, TCHAR* argv[])
{
	CmdParser cmdParser;
	configure_parser(cmdParser);
	cmdParser.parseCmdLine(argc, argv);

	if (cmdParser.isPrintHelp())
	{
		print_hep_message(cmdParser);
	}
	else
	{
        try
        {
            tstring target_path = cmdParser.get<tstring>(_T("t"));
            tstring db_path = cmdParser.get<tstring>(_T("d"));
            int log_level = cmdParser.get<int>(_T("l"));
            atomic_bool isDebuggerOn = true;

            RECORD recorder = RECORD(db_path, log_level); 
            
            thread debugThread([&target_path, &isDebuggerOn]() {
                sharedDebugger = make_shared<Debug>(target_path);
                sharedDebugger->loop(&isDebuggerOn); 
            });
            
            recorder.log(&isDebuggerOn); 
            debugThread.join(); 
        }
        catch (const exception& e)
        {
            cerr << "An error occurred: " << e.what() << endl;
        }
	}
	return 0;
};