#pragma once

#include <Stamina\Semaphore.h>

#include "tables.h"

#define  BETA_LOGIN      (tColId)0 //pch
#define  BETA_PASSMD5    1 //pch
#define  BETA_AFIREWALL   4
#define  BETA_AMODEM      5
#define  BETA_AWBLINDS   8
#define  BETA_ANONYMOUS  9
#define  BETA_ANONYMOUS_LOGIN 10
#define  BETA_LAST_REPORT 11
#define  BETA_URID 12
#define  BETA_LOGIN_CHANGE 13 //pch
#define  BETA_LAST_STATICREPORT 14 //pch

// Data trzymana jest w identyfikatorze wiersza jako _time64 / 86400
#define  STATS_DATE (tColId)0 // _time64 / 86400
#define  STATS_STARTCOUNT 1
#define  STATS_UPTIME     2
#define  STATS_MSGSENT    3
#define  STATS_MSGRECV    4
#define  STATS_REPORTED   5

#define  REP_DATE     (tColId)0
#define  REP_TYPE     1 //int
#define  REP_TITLE    2
#define  REP_MSG      3
#define  REP_OS       4
#define  REP_PLUGS    5
//#define  REP_VERSIONINFO  6
#define  REP_INFO    7
#define  REP_LOG    8
#define  REP_REPORTED    9
#define  REP_DIGEST    10
#define  REP_VERSION  11

#define REPTYPE_PROPOSAL 0
#define REPTYPE_BUG 1
#define REPTYPE_EXCEPTION 0x10
#define REPTYPE_ERROR 0x11

//#define URL_BETA_CENTRAL ""
#define BETA_SERVER_COUNT 2
#define BETA_SERVER_START 1

#define PATH_BETA dataPath + "beta"
#define FILE_BETA dataPath + "beta\\beta.dtb"
#define FILE_REPORTS dataPath + "beta\\beta_reports.dtb"
#define FILE_STATS dataPath + "beta\\beta_stats.dtb"

#define URL_BETA "http://www.konnekt.info"
#define URL_HELP "doc\\index.html"
#define URL_REGISTER "http://www.konnekt.info/kweb/register.php"

#define BLANK_PASS "_____"


namespace Konnekt {
	namespace Beta {

		struct ErrorReport {
			CStdString msg;
			CStdString info;
		};

		const int reportInterval = 86400 * 3; // min. czas pomiêdzy raportami w sekundach - 3 dni
		const int anonymousReportInterval = 86400 * 7; // min. czas pomiêdzy raportami w sekundach - 3 dni
		const int minimumReportInterval = 20 * 60; // absolutnie min. czas pomiêdzy raportami w sekundach - 20 minut
		const int anonymousMinimumReportInterval = 60 * 60; // absolutnie min. czas pomiêdzy raportami w sekundach - 20 minut
		const int checkInterval = 5 * 60; // czas pomiêdzy próbami w razie braku po³¹czenia z sieci¹...

		extern string betaLogin;
		extern string betaPass;
		extern bool anonymous;
		extern __time64_t lastStat;

		extern Tables::tTableId tableBeta;
		extern Tables::tTableId tableStats;
		extern Tables::tTableId tableReports;

		extern bool hwndBeta;
		extern bool loggedIn;

		extern Stamina::Semaphore sendSemaphore;


		void init();
		void setStats(__time64_t forTime);
		VOID CALLBACK setStats(LPVOID lParam,  DWORD dwTimerLowValue ,  DWORD dwTimerHighValue );
		void showBeta(bool modal=false);
		void showReport();
		void send(bool force = false);
		/** wywo³uje send z opóŸnieniem... */
		void sendPostponed();
		void close();

		int report(int type , const char * title , const char * msg , const char * digest, const CStdString& log, bool send);

		string makeDigest(const string& txt);

		string info_log();
		__int64 info_serialSystem();
		__int64 info_serialInstance();
		string info_serial();
		string info_other(bool extended=false);
		string info_os();
		string info_plugins(bool withFilenames);
		void imdigest(string & msg);
		void  stackdigest(string & msg);

		void lockdown();
		void cutoffWindow(HWND hwnd);

		void errorreport(int type , const CStdString & title , CStdString & msg, const char * digest, const CStdString& log);


		int CALLBACK BetaDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
		int CALLBACK ReportDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
		int CALLBACK ErrorDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
		LRESULT DummyProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);


		void makeReport(CStdString filename, bool open, bool usedate, bool silent);
	};
};