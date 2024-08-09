#include "debugger.h"
#include "record.h"
#include "cmdparser.h"
#include "cmdparser.h"
#include <thread>

using namespace cmdutil;
using namespace debugger;
using namespace record;

shared_ptr<Debug> sharedDebugger; // 디버거의 전역 접근을 위해 shared_ptr 사용, 수명은 메인 스레드 종료시까지

// 옵션 등록
void configure_parser(CmdParser& parser)
{
	parser.set_required<tstring>(_T("t"), _T("target_path"), _T("The target's path"));
	parser.set_required<tstring>(_T("d"), _T("db_path"), _T("The DB's path"));
	parser.set_optional<int>(_T("l"), _T("log_level"), 1, _T("log_level: TEXT for 1, DB for 2"));
};

// 도움말 출력
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

            //record 객체 생성, 메인 스레드에서 동작
            RECORD recorder = RECORD(db_path, log_level); // log_level : 1 -> text, else -> sqlite3 db
            
            // 디버그 스레드 생성 및 실행
            thread debugThread([&target_path, &isDebuggerOn]() {
                sharedDebugger = make_shared<Debug>(target_path);
                sharedDebugger->loop(&isDebuggerOn); // pc를 이용하여 DB에서 함수 검색 시작
            });
            
            recorder.log(&isDebuggerOn); // pc를 이용하여 DB에서 함수 검색 시작
            debugThread.join(); // 스레드가 완료될 때까지 기다림
        }
        catch (const exception& e)
        {
            cerr << "An error occurred: " << e.what() << endl;
        }
	}
	return 0;
};