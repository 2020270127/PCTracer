#include "dllparser.h"
#include "cmdparser.h"
#include "typedef.h"
#include "logger.h"
//#include <atomic>

using namespace cmdparser;
using namespace std;


void configure_parser(CmdParser& parser)
{
	parser.set_required<tstring>(_T("d"), _T("directory"), _T("The target dll's directory."));
	parser.set_optional<tstring>(_T("n"), _T("DB name"), _T("DLL"), _T("The target dll's directory."));

};

void print_hep_message(CmdParser& parser)
{
	tcout << parser.getHelpMessage(_T("TRACER"));
};

int _tmain(int argc, TCHAR* argv[])
{
	CmdParser cmdParser;
	logging::Logger Logger_;

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
			tstring targetDirectory = cmdParser.get<tstring>(_T("d"));
			tstring DBName = cmdParser.get<tstring>(_T("n")) + ".db";
			dllParser dllParser_(DBName);
			

			dllParser_.parseDLLEATRecursively(targetDirectory);
		}
		catch (runtime_error e)
		{
			Logger_.log(format(_T("Error : {}\n"), e.what()));
			print_hep_message(cmdParser);
		}
	}
	return 0;
};



