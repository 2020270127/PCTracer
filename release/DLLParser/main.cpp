#include "dllparser.h"
#include "strconv.h"
#include "cmdparser.h"
#include "typedef.h"


using namespace cmdparser;
using namespace std;


// 옵션 등록
void configure_parser(CmdParser& parser)
{
	parser.set_required<tstring>(_T("d"), _T("directory"), _T("The target dll's directory."));

};

// 도움말 출력
void print_hep_message(CmdParser& parser)
{
	tcout << parser.getHelpMessage(_T("TRACER"));
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
			dllParser dllparser;

			tstring targetDirectory = cmdParser.get<tstring>(_T("d"));
			dllparser.ParseDllRecursively(targetDirectory);
		}
		catch (runtime_error ex)
		{
			cout << format("Error : {}\n\n", ex.what());
			print_hep_message(cmdParser);
		}
	}
	return 0;
};



