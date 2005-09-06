//#ifdef __BETA

#include "stdafx.h"


#include <Stamina\Timer.h>
#include <Stamina\Internet.h>
#include <Stamina\Time64.h>
#include <Stamina\RegEx.h>
#include <Stamina\Helpers.h>


#include "main.h"
#include "beta.h"
#include "resources.h"
#include "messages.h"
#include "plugins.h"
#include "pseudo_export.h"
#include "imessage.h"
#include "tables.h"
#include "debug.h"
#include "connections.h"
#include "threads.h"


#define URL_BETA_CENTRAL "http://beta%d.konnekt.info/beta_central.php"
//#define URL_BETA_CENTRAL "http://test.konnekt.info/beta/beta_central.php"

using Stamina::RegEx;

using Stamina::ExceptionString;

using namespace Stamina;

using Tables::oTable;
using Tables::oTableImpl;

using Tables::TableLocker;

/*

Nowy beta_central.php (konnekt-beta login). Wszystkie dane wrzucane przy pomocy
jednego zapytania, raz na jakiœ czas...

Na serwer wysy³ane jest:
login=
pass=[md5]
anonymous=
os=
nfo=
serial=
version=HEX
// komponenty
plugins=SIG HEX,SIG HEX,SIG HEX
dev=1/0
#C[][n]=nazwa
#C[][s]=sig
#C[][v]=wersja
#C[][vh]=wersja hex'em
// statystyki
S[][date]=
S[][id]=
S[][uptime]=
S[][count]=
S[][msent]=
S[][mrecv]=
// raporty
R[][date]=
R[][id]=
R[][version]=
R[][type]=
R[][title]=
R[][msg]=
R[][os]=
#R[][C]=komponenty
R[][plugins]=
R[][nfo]=
R[][log]=
R[][digest]=



*/


namespace Konnekt { namespace Beta {

	const int logReadout = 10000;
	const int logSendout = 255;


	bool hwndBeta=0;
	bool loggedIn=false;

	//HANDLE timerBeta=0;
	//HANDLE timerBeta24=0;


	int retryTime=0;
	const int centralTimeout=1000 * 60;
	
	int sendRetries = 0;

	string betaLogin="";
	string betaPass="";
	bool anonymous=false;

	Stamina::Semaphore sendSemaphore (0, 1);
	Stamina::Semaphore statSemaphore (0, 1);
	Stamina::TimerBasic statTimer;
	
	const char ANONYM_CHAR = '^';
	/*
	#define URL_BETA_LOGIN  "~stamina/konnekt/k_login.php"
	#define URL_BETA_REPORT "~stamina/konnekt/k_report.php"
	#define URL_BETA_SERVER "jego.stamina.eu.org"
	*/
	//#define URL_BETA_LOGIN  "k_login.php"
	//#define URL_BETA_REPORT "k_report.php"
	//#define URL_BETA_SERVER "test.konnekt.info"



	tTableId tableBeta;
	tTableId tableStats;
	tTableId tableReports;

	DWORD time_started;
	__time64_t lastStat;


	void init() {

		using namespace Tables;

		time_started = GetTickCount();
		lastStat = _time64(0);

		std::string password = stringf("%16I64X", info_serialInstance());

		oTable beta = registerTable(Ctrl, "Beta", optPrivate | optAutoLoad | optAutoSave | optAutoUnload | optDiscardLoadedColumns | optUsePassword | optSilent | optGlobalData);
		beta->setTablePassword(password.c_str());
		beta->setFilename("beta.dtb");
		beta->setDirectory(PATH_BETA);
		tableBeta = beta->getTableId();

		beta->setColumn(BETA_LOGIN , DT_CT_PCHAR|DT_CF_CXOR , "");
		beta->setColumn(BETA_PASSMD5 , DT_CT_PCHAR|DT_CF_CXOR , "");
		beta->setColumn(BETA_AFIREWALL , DT_CT_INT , 0);
		beta->setColumn(BETA_AMODEM , DT_CT_INT , 0);
		beta->setColumn(BETA_AWBLINDS , DT_CT_INT , 0);
		beta->setColumn(BETA_ANONYMOUS_LOGIN , DT_CT_PCHAR|DT_CF_CXOR , "");
		beta->setColumn(BETA_ANONYMOUS , DT_CT_INT , 1);
		beta->setColumn(BETA_LAST_REPORT , DT_CT_64 , 0);
		beta->setColumn(BETA_URID , DT_CT_INT , 0);
		beta->setColumn(BETA_LOGIN_CHANGE, DT_CT_64, 0);
		beta->setColumn(BETA_LAST_STATICREPORT, DT_CT_64, 0);


		oTable stats = registerTable(Ctrl, "Stats", optPrivate | optAutoLoad | optAutoSave | optAutoUnload | optDiscardLoadedColumns | optUsePassword | optSilent | optGlobalData);
		stats->setTablePassword(password.c_str());
		stats->setFilename("beta_stats.dtb");
		stats->setDirectory(PATH_BETA);
		tableStats = stats->getTableId();

		stats->setColumn(STATS_DATE , DT_CT_64 , 0);
		stats->setColumn(STATS_STARTCOUNT , DT_CT_INT , 0);
		stats->setColumn(STATS_UPTIME , DT_CT_INT , 0);
		stats->setColumn(STATS_MSGRECV , DT_CT_INT , 0);
		stats->setColumn(STATS_MSGSENT , DT_CT_INT , 0);
		stats->setColumn(STATS_REPORTED , DT_CT_INT , 0);


		oTable reports = registerTable(Ctrl, "Reports", optPrivate | optAutoLoad | optAutoSave | optAutoUnload | optDiscardLoadedColumns | optUsePassword | optSilent | optGlobalData);
		reports->setTablePassword(password.c_str());
		reports->setFilename("reports.dtb");
		reports->setDirectory(PATH_BETA);
		tableReports = reports->getTableId();

		reports->setColumn(REP_DATE , DT_CT_64);
		reports->setColumn(REP_TYPE , DT_CT_INT);
		reports->setColumn(REP_TITLE , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_MSG , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_OS , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_PLUGS , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_INFO , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_LOG , DT_CT_PCHAR | DT_CT_CXOR);
		reports->setColumn(REP_REPORTED , DT_CT_INT , 0);
		reports->setColumn(REP_DIGEST , DT_CT_PCHAR , 0);
		reports->setColumn(REP_VERSION , DT_CT_INT);

		if (beta->getRowCount() == 0) beta->addRow();

		setStats(0);

		if (!*beta->getCh(0 , BETA_LOGIN) && !beta->getInt(0 , BETA_ANONYMOUS)) showBeta();
		betaLogin = beta->getStr(0, BETA_LOGIN);
		betaPass = beta->getStr(0, BETA_PASSMD5);
		anonymous = beta->getInt(0 , BETA_ANONYMOUS);
		if (!betaLogin.empty() && betaLogin[0]!=ANONYM_CHAR) {
			anonymous = false;
		} // Jawna nieprawid³owoœæ :)
	}

	void showBeta(bool modal) {
		if (!hwndBeta) {
			hwndBeta=1;
			DialogBox(Stamina::getInstance() , MAKEINTRESOURCE(IDD_BETA)
				, 0 , (DLGPROC)BetaDialogProc);
		}               
		//  else SetForegroundWindow(hwndBeta);
	}
	void showReport() {
		if (anonymous) return;
		ShowWindow(CreateDialog(Stamina::getInstance() , MAKEINTRESOURCE(IDD_REPORT)
			, 0 , (DLGPROC)ReportDialogProc)
			,SW_SHOW);

	}

	void close() {
		static bool once = false;
		if (once) return;
		once = true;
		statTimer.stop();
		setStats(0);
	}

	void setStats(__time64_t forTime) {

		Stamina::SemaphoreCtx sctx (statSemaphore);

		static bool firstStat = true;
		static int lastMsgSent = 0;
		static int lastMsgRecv = 0;
		if (forTime == 0)
			forTime = lastStat;
		oTableImpl stats(tableStats);

		Tables::TableLocker lock (stats, Tables::allRows);

		// Odnajdujemy/tworzymy aktualn¹ datê...
		// 86400 sek/dzieñ
		int days = (int)(forTime / 86400);
		int id = DataTable::flagId(days);
		if (DataTable::unflagId(id) > colIdUniqueFlag) {
			return; // coœ siê nie zgadza!!!
		}
		if (stats->getRowPos(id) == Tables::rowNotFound)
			stats->addRow(id);
		int row = stats->getRowPos(id); // ¿eby by³o szybciej...
		if (firstStat) {
			stats->setInt(row, STATS_STARTCOUNT, stats->getInt(row , STATS_STARTCOUNT) + 1);
		}
		int uptime;
		uptime = (GetTickCount() - time_started) / 1000;
		if (uptime > 86400)
			uptime = 86400;
		if (uptime > 0)
			stats->setInt(row , STATS_UPTIME , stats->getInt(row , STATS_UPTIME) + uptime);
		stats->setInt(row , STATS_MSGSENT , stats->getInt(row , STATS_MSGSENT) + Messages::msgSent);
		stats->setInt(row , STATS_MSGRECV , stats->getInt(row , STATS_MSGRECV) + Messages::msgRecv);
		stats->setInt64(row , STATS_DATE , (days * 86400) + 14400 );
		// 

		// Zerujemy wszystkie liczniki
		time_started = GetTickCount();
		lastStat = _time64(0);
		lastMsgRecv = Messages::msgRecv;
		lastMsgSent = Messages::msgSent;
		// Zamawiamy timer, do wykonania nastêpnego dnia...
		statTimer.start(setStats, 0, (86400 - (lastStat % 86400)) * 1000);
		IMLOG("Statystyka id = %d; uptime = %d; msgSent = %d; msgRecv = %d" , id , uptime , Messages::msgSent , Messages::msgRecv);

		stats->save();
		
		IMLOG("- Beta report saved");
		firstStat = false;
	}

	VOID CALLBACK setStats(LPVOID lParam,  DWORD dwTimerLowValue ,  DWORD dwTimerHighValue ) {
		setStats(0);
	}

	string info_plugins() {
		string s;
		for (unsigned int i=0; i<Plug.Plug.size(); i++) {
			//FileVersionInfo(Plug[i].file.c_str() , TLS().buff);
			if (!s.empty()) {
				s += ',';
			}
			s += RegEx::doReplace("/[^a-zA-Z0-9_]/", "_", Plug[i].sig.c_str());
			s += " ";
			s += inttostr(Plug[i].version.getInt(),16);
		}
		return s;
	}

	string info_os() {
		string str="";
		OSVERSIONINFO ovi;
		ovi.dwOSVersionInfoSize = sizeof(ovi);
		if (!GetVersionEx(&ovi)) return "";
		switch (ovi.dwPlatformId) {
	case VER_PLATFORM_WIN32_WINDOWS:
		if (ovi.dwMinorVersion<10) str+="Win95";
		else if (ovi.dwMinorVersion<90) str+="Win98";
		else str+="WinME";
		break;
	case VER_PLATFORM_WIN32_NT:
		if (ovi.dwMajorVersion==5) {
			if (ovi.dwMinorVersion==0) str+="Win2000";
			else if (ovi.dwMinorVersion==1) str+="WinXP";
		} else str+="WinNT";
		break;
		}
		str+=" ";
		str+=ovi.szCSDVersion;
		str+=" ";
		str+=inttostr(ovi.dwMajorVersion);
		str+=".";
		str+=inttostr(ovi.dwMinorVersion);
		str+=".";
		str+=inttostr(ovi.dwPlatformId==VER_PLATFORM_WIN32_NT?ovi.dwBuildNumber:LOWORD(ovi.dwBuildNumber));
		return str;
	}

	string info_other(bool extended) {
		HKEY hKey=HKEY_LOCAL_MACHINE;
		string str = "";
		/*
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Internet Explorer",0,KEY_READ,&hKey))
		{
			str += "IE=" + RegQuerySZ(hKey , "Version") + "\n";
		}	
		RegCloseKey(hKey);
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,string("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0").c_str(),0,KEY_READ,&hKey))
		{
			str+="CPU=";
			str+=RegQuerySZ(hKey,"VendorIdentifier");
			str+=" , ";
			str+=RegQuerySZ(hKey,"Identifier");
			str+=" , ";
			int mhz = RegQueryDW(hKey , "~MHz");
			str+=inttostr(mhz);
			str+=" Mhz\n";
		}
		RegCloseKey(hKey);
		if (Beta.getint(0 , BETA_AFIREWALL))
			str+="Firewall=1\n";
		if (Beta.getint(0 , BETA_AMODEM))
			str+="Dial-up=1\n";
		if (Beta.getint(0 , BETA_AWBLINDS))
			str+="GUIaddon=1\n";
		if (extended) {
			DWORD time_closed;
			time_closed=GetTickCount();
			str+="Uptime="+inttostr((time_closed > time_started ? (time_closed - time_started) : (0x3FFFFFFF))/1000 )+"\n";
			str+="MsgSent="+inttostr(Messages::msgSent)+"\n";
			str+="MsgRecv="+inttostr(Messages::msgRecv)+"\n";
		}
		*/
		/*  time_t time_tmp; time(&time_tmp);
		cTime64 ltime(gmtime(&time_tmp));
		str+="LTime="+ltime.strftime("%d-%m-%Y %H:%M:%S GMT")+"\n";*/
		return str;
	}

	__int64 info_serialSystem() {
		DWORD serialNumberSystem = 0;
		if (GetVolumeInformation("C:\\" , 0 , 0 , &serialNumberSystem , 0 , 0 , 0 , 0)) {
			return MD5_64(inttostr(serialNumberSystem).c_str());
		} else {
			return 0;
		}
	}

	__int64 info_serialInstance() {
		DWORD serialNumberInstance = 0;
		CStdString instancePath = appPath;
		instancePath = Stamina::RegEx::doGet("#^((?:[a-z]:\\\\)|(?:\\\\{2}.+?\\\\.+?\\\\))#i", instancePath, 1, instancePath);
		if (GetVolumeInformation(instancePath , 0 , 0 , &serialNumberInstance , 0 , 0 , 0 , 0)) {
			return MD5_64(inttostr(serialNumberInstance) + appPath);
		} else {
			return info_serialSystem();
		}
	}

	// Zwraca "numer seryjny" kopii Konnekta na podstawie kilku charakterystycznych wartoœci komputera i œcie¿ki
	string info_serial() {
		CStdString serial;
		serial.Format("%16I64X%16I64X", info_serialSystem(), info_serialInstance());
		//return serialSystem.substr(0, 16) + serialInstance.substr(0, 16);
		return serial;
	}

	string info_log(){
		string s;
#ifdef __DEBUG
		if (Debug::logFile) {
			FILE * f = fopen(Debug::logFileName , "rt");
			if (!f) return "";
			fseek(f , -logReadout , SEEK_END);
			if (fread(stringBuffer(s, logReadout) , 1 , logReadout , f)) {
				stringRelease(s);
			}
			fclose(f);
		}
#endif
		return s;
	}


	unsigned int sendThread(bool force) {
	
		Stamina::randomSeed();

		Stamina::SemaphoreCtx sctx (sendSemaphore);

		Tables::oTableImpl beta(tableBeta);

		if (!beta->getRowCount()) beta->addRow();

		// Jakiœ BETA_URID musi byæ ustawiony...
		if (beta->getInt(0 , BETA_URID) == 0) {
			int newUrid = (_time64(0) - reportInterval) & 0xFFFFFF;
			beta->setInt(0, BETA_URID, newUrid);
			// od razu tworzymy katalog
			if (_access(PATH_BETA, 0)) {
				Stamina::createDirectories(PATH_BETA);
			}
			beta->save();
			beta->resetData();
			beta->load(true);
			if (beta->getInt(0, BETA_URID) != newUrid) {
				K_ASSERT_MSG(0, "Z³y BETA_URID!");
				IMDEBUG(DBG_ERROR, "- [BT]Problem with beta.dtb");
				return 0;
			}
		}


		Tables::oTableImpl reports(tableReports);

		bool retry=false;
		// Gdy nie robimy Resend, nie ma raportów i nie minê³o wystarczaj¹co du¿o czasu, olewamy sprawê
		int interval = abs((int)(_time64(0) - beta->getInt64(0 , BETA_LAST_REPORT)));

		bool anonymous = beta->getInt(0, BETA_ANONYMOUS);
		int currentMinimumInterval = anonymous ? anonymousMinimumReportInterval : minimumReportInterval;
		int currentInterval = anonymous ? anonymousReportInterval : reportInterval;

		if (!force) {
            // szukamy nowszych raportów
			bool gotNewReports = false;
			__int64 lastReport = beta->getInt64(0 , BETA_LAST_REPORT);
			for (unsigned int i=0; i<reports->getRowCount(); i++) {
				if (reports->getInt64(i, REP_DATE) > lastReport)
					gotNewReports = true;
			}

			if ( (interval < currentMinimumInterval) || ( (interval < currentInterval) && gotNewReports == false ) ) {
				IMLOG("- [BT]Nothing to report, interval = %d", interval);
				return 0;
			}

		}
		sendRetries++;

		Tables::oTableImpl stats(tableStats);

		//   IMLOG("- Beta comm %s" , lParam?"retry":"first try");
		FILE * log;
		try {
			log = fopen(Debug::logPath + "reports.log" , "at");


			int reportsCount = 0;
			int statsCount = 0;
			string frmdata;
			{
				TableLocker betaLock(beta);
				TableLocker reportsLock(reports);
				TableLocker statsLock(stats);

				IMLOG("* BT Zbieram dane...");
				// login , pass		
				frmdata = "login="+urlEncode(beta->getStr(0 , BETA_LOGIN))
					+"&pass="+urlEncode(beta->getStr(0 , BETA_PASSMD5));
				// urid (Unique Report ID) - do wykrywania powtórnych wys³añ
				int urid = beta->getInt(0 , BETA_URID);
				frmdata += "&urid=" + inttostr(urid, 16);
				// anonymous
				if (beta->getStr(0 , BETA_ANONYMOUS_LOGIN).empty() == false) {
					frmdata+="&anonymous="+urlEncode(beta->getStr(0 , BETA_ANONYMOUS_LOGIN));
				}
				// os , nfo
				frmdata += "&os=" + urlEncode(info_os());
				frmdata += "&nfo=" + urlEncode(info_other());
				frmdata += "&serial=" + urlEncode(info_serial());
				frmdata += "&version=" + inttostr(versionNumber , 16);

	#ifdef __DEV
				frmdata+="&dev=1";
	#endif
				frmdata += "&plugins=" + urlEncode(info_plugins());

				/* Fakt ¿e mamy login w pewien sposób upewnia nas, ¿e dostajemy odpowiedzi... */

				if (beta->getStr(0 , BETA_LOGIN).length() > 0 || beta->getStr(0, BETA_LAST_REPORT).length() > 0) {
					// Statystyki
					for (unsigned int i=0; i<stats->getRowCount(); i++) {
						if ((int)(stats->getInt64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
						if (stats->getInt(i, STATS_REPORTED)) {
							continue;
						}
						statsCount ++;
						string var = "&S[" + inttostr(i) + "]";
						frmdata += var + "[id]=" + inttostr(stats->getRowId(i), 16);
						frmdata += var + "[date]=" + Time64(stats->getInt64(i , STATS_DATE)).strftime("%Y-%m-%d"); 
						frmdata += var + "[uptime]=" + inttostr(stats->getInt(i , STATS_UPTIME));
						frmdata += var + "[count]=" + inttostr(stats->getInt(i , STATS_STARTCOUNT));
						frmdata += var + "[msent]=" + inttostr(stats->getInt(i , STATS_MSGSENT));
						frmdata += var + "[mrecv]=" + inttostr(stats->getInt(i , STATS_MSGRECV));
					}
					// Raporty
					for (unsigned int i=0; i<reports->getRowCount(); i++) {
						if (reports->getInt(i, REP_REPORTED)) {
							continue;
						}
						reportsCount ++;
						string var = "&R[" + inttostr(i) + "]";
						frmdata += var + "[id]=" + inttostr(reports->getRowId(i), 16);
						frmdata += var + "[date]=" + Time64(reports->getInt64(i , REP_DATE)).strftime("%Y-%m-%d"); 
						frmdata += var + "[type]="+inttostr(reports->getInt(i , REP_TYPE));
						frmdata += var + "[title]="+urlEncode(reports->getStr(i , REP_TITLE));
						frmdata += var + "[msg]="+urlEncode(reports->getStr(i , REP_MSG));
						frmdata += var + "[os]="+urlEncode(reports->getStr(i , REP_OS));
						frmdata += var + "[nfo]="+urlEncode(reports->getStr(i , REP_INFO));
						CStdString log = reports->getStr(i , REP_LOG);
						if (log.length() > logSendout) {
							log.erase(0, log.length() - logSendout);
						}
						frmdata += var + "[log]=" + urlEncode(log);
						frmdata += var + "[digest]=" + urlEncode(reports->getStr(i , REP_DIGEST));
						frmdata += var + "[version]="+inttostr(reports->getInt(i , REP_VERSION), 16);
						frmdata += var + "[plugins]="+urlEncode(reports->getStr(i , REP_PLUGS));
					}
				} // have login...
			} // unlock

			IMLOG("* BT Próbujê po³¹czyæ...");

			CStdString url;
			srand(time(0));
			url.Format(URL_BETA_CENTRAL, Stamina::random(1, BETA_SERVER_COUNT) );
			url += "?";
			if (beta->getStr(0 , BETA_LOGIN)[0] == 0) {
				url += "nologin=1";
			} else if (beta->getInt(0 , BETA_ANONYMOUS)){
				url += "anon=1";
			} else {
				url += "auth=1";
			}
			if (statsCount) {
				url += "&stats=1";
			}
			if (reportsCount) {
				url += "&reps=1";
			}
			if (beta->getInt64(0, BETA_LOGIN_CHANGE) > beta->getInt64(0, BETA_LAST_REPORT)) {
				url += "&newauth=1";
			}
			// doklejamy dodatkowe informacje do requesta...


			IMDEBUG(DBG_LOG, "* BT - wysylamy na %s", url.c_str());

			TCHAR hdrs[] = _T("Content-Type: application/x-www-form-urlencoded");
			//      TCHAR accept[] =
			//         _T("Accept: */*");

			Stamina::oInternet internet ( new Stamina::Internet((HINTERNET) IMCore(&sIMessage_2params(IMC_HINTERNET_OPEN , (int)"Konnekt-beta" , 0))) );

			internet->setReadTimeout(centralTimeout);
			internet->setWriteTimeout(centralTimeout);

			Stamina::oRequest request = internet->handlePostWithRedirects(url, hdrs, frmdata);
			if (request->getStatusCode() != 200) {
				throw ExceptionString("Z³a odpowiedŸ serwera! " + request->getStatusText());
			}

			Stamina::RegEx response;
			response.setSubject(request->getResponse());

			{ // lock
				TableLocker betaLock(beta);
				TableLocker reportsLock(reports);
				TableLocker statsLock(stats);

				IMLOG("* BT Serwer odpowiedzia³ (%d b) \"%.100s...\"" , response.getSubjectRef().length(), response.getSubjectRef().c_str());

				if (log) { // logujemy wynik...
					CStdString sent;
					RegEx r;
					sent = "&" + urlDecode(frmdata);
					r.setSubject(sent);
					r.replaceItself("/\n/","\n\t\t\t");
					r.replaceItself("/&(.+?)=/i","\n\t\t$1\t");
					r.replaceItself("/pass\t.*\n/i","pass\t...\n");
					Stamina::Time64 now(true);
					fprintf(log , "%s Zalogowa³em siê do systemu BETA i wys³a³em: \n\t\t%s\n" ,  now.strftime("%d-%m-%Y %H:%M:%S").c_str() , r.getSubject().c_str());
					fprintf(log , "\tW odpowiedzi przysz³o: \n\t\t%s\n\n" , r.replace("/\n/","\n\t\t",response.getSubject().c_str()).c_str());
				}

				if (!response.match("/^BetaCentral /")) {
					IMDEBUG(DBG_ERROR, "* BT Z³a odpowiedŸ! \"%.500s...\"", response.getSubjectRef().c_str());
					throw ExceptionString("Bad Format");
				} 
				// Zbieramy komunikaty o b³êdach...
				CStdString error_msgs;
				response.setPattern("/^Error\\t([^\\r\\n]+)$/m");
				while (response.match_global()>1) {
					error_msgs+="\r\n - "+response[1];
				}
				if (!error_msgs.empty()) {
					error_msgs = "Podczas przetwarzania raportów, system Beta zwróci³ nast. b³êdy:\r\n" + error_msgs + "\r\n\r\nNajlepiej poinformuj nas o tym na FORUM.";
					cMessage m;
					m.type = MT_SERVEREVENT;
					m.body = (char*)error_msgs.c_str();
					m.time = _time64(0);
					m.flag = MF_HANDLEDBYUI;
					sMESSAGESELECT ms;
					ms.id = IMessage(IMC_NEWMESSAGE , 0 , 0 , (int)&m);
					if (ms.id)
						IMessage(IMC_MESSAGEQUEUE , 0 , 0 , (int)&ms);
				}
				response.reset();

				if (!response.match("/^End$/m")) {
					throw ExceptionString("Reply not closed");
				} // Jezeli nie ma na koncu End'a, znaczy ze najpewniej dostajemy zly wynik!
				if (response.match("/^Info=\"(.+)\"$/m")) {
					CStdString reason = response[1];
					reason.Replace("\\n" , "\n");
					CStdString URL = "";
					if (response.match("/^InfoURL=\"(.+)\"$/m") > 0)
						URL = response[1];
					int r = MessageBox(0 , ("Informacja od systemu Beta.\r\n\r\n" + reason).c_str() , "Konnekt@BETA" , MB_ICONINFORMATION|(URL.empty()?MB_OK:MB_YESNO)|MB_TASKMODAL);
					if (!URL.empty() && r==IDYES) 
						ShellExecute(0 , "open" , URL , "" , "" , SW_SHOW);
				}
				if (response.match("/^Unauthorized$/m")) {
					beta->setStr(0 , BETA_LOGIN , "");
					beta->setStr(0 , BETA_PASSMD5 , "");
					beta->setInt(0 , BETA_ANONYMOUS , 0);

					betaLogin = "";
					betaPass = "";
					anonymous = true;

					beta->save();
					response.match("/^Reason=\"(.+)\"$/m");
					CStdString reason = response[1];
					reason.Replace("\\n" , "\n");
					CStdString URL = "";
					if (response.match("/^ReasonURL=\"(.+)\"$/m") > 0)
						URL = response[1];
					/*
					int r = MessageBoxTimeout(0 , "Jesteœ nieupowa¿niony do u¿ywania wersji Beta!\r\nProgram zostanie zamkniêty w przeci¹gu minuty.\r\n\r\n" + reason , "Konnekt@BETA" , MB_ICONERROR|(URL.empty()?MB_OK:MB_YESNO)|MB_TASKMODAL , 60000);
					*/
					int r = MessageBox(0 , reason , "Konnekt@BETA" , MB_ICONERROR|(URL.empty()?MB_OK:MB_YESNO)|MB_TASKMODAL);
					if (!URL.empty() && r==IDYES) 
						ShellExecute(0 , "open" , URL , "" , "" , SW_SHOW);
					//deinitialize();
					//loggedIn = false;
				} else {
					//loggedIn = true;
				}
				loggedIn = true; // zawsze zalogowany...

				if (loggedIn) {
					response.reset();
					// User zosta³ zarejestrowany jako anonim...
					if (response.match("/^Registered=\"(.+),(.*)\"$/m")) {
						beta->setStr(0 , BETA_LOGIN , response[1].c_str());
						beta->setStr(0 , BETA_PASSMD5 , response[2].c_str());
						beta->setInt(0 , BETA_ANONYMOUS , 1);
						betaLogin = response[1];
						betaPass = response[2];
						anonymous = true;
					}
					// User ju¿ nie jest anonimem...
					if (response.match("/^Unregistered$/m")) {
						beta->setStr(0 , BETA_ANONYMOUS_LOGIN , "");
					}
					if (response.match("/^Reported$/m")) {
						beta->setInt64(0 , BETA_LAST_REPORT , _time64(0));
						// ¯eby za ka¿dym razem sprawdza³ czy mo¿e pisaæ po beta.dtb
						//Beta.setint(0 , BETA_URID , _time64(0) & 0xFFFFFF);
						beta->setInt(0 , BETA_URID , 0);
					}
					if (response.match("/^StatsStoredLast (\\d+)$/m")) {
						__int64 last = (int)(_atoi64(response[1].c_str()) / 86400) * 86400;
						for (unsigned int i=0; i<stats->getRowCount(); i++) {
							if ((int)(stats->getInt64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)
								|| stats->getInt64(i , STATS_DATE) > last
								) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
							stats->removeRow(i);
							i--;
						}
					} else if (response.match("/^StatsStored$/m")) {
						for (unsigned int i=0; i<stats->getRowCount(); i++) {
							if ((int)(stats->getInt64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
							stats->removeRow(i);
							i--;
						}
					}
					if (response.match("/^ReportsStoredLast (\\d+)$/m")) {
						__int64 last = _atoi64(response[1].c_str());
						for (unsigned int i=0; i<reports->getRowCount(); i++) {
							if (reports->getInt64(i , REP_DATE) > last) continue; // pomijamy
							//Reports.deleterow(i);
							//i--;
							reports->setInt(i, REP_REPORTED, 1);
						}
					} else if (response.match("/^ReportsStored$/m")) {
	//					Reports.clearrows();
						for (unsigned int i=0; i<reports->getRowCount(); i++) {
							reports->setInt(i, REP_REPORTED, 1);
						}

					}
					if (response.match("/^Retry=(\\d+)$/m")) {
						retry = true;
					}
				} // loggedIn
			} // Locked
		} catch (Stamina::ExceptionInternet e)	{
			IMLOG("![BT] Wyst¹pi³ b³¹d sieci GLE:%d WSA:%d [%s]!" , GetLastError() , WSAGetLastError(), e.getReason().c_str());
			retry=true; // mo¿emy spróbowaæ za jakiœ czas skoro teraz s¹ byæ mo¿e problemy z sieci¹...
		} catch (ExceptionString e)	{
			IMLOG("![BT] Wyst¹pi³ b³¹d GLE:%d [%s]!" , GetLastError() , e.getReason().c_str());
			retry=false; // skoro dostaliœmy jak¹œ odpowiedŸ i by³a uszkodzona - nie pytamy na razie wiêcej!
			// i ustawiamy ¿eby nie pyta³ przez najbli¿szy czas równie¿...
			beta->setInt64(0 , BETA_LAST_REPORT , _time64(0));
			// 
			//Beta.setint(0 , BETA_URID , _time64(0) & 0xFFFFFF);
			beta->setInt(0 , BETA_URID , 0);
		}

		beta->save();
		stats->save();
		reports->save();

		if (log)
			fclose(log);

		if (retry) {
			sendPostponed();
		}
		return 0;
	}


	void sendPostponed() {
		if (sendRetries > 1) {
			return;
		}
		if (Konnekt::mainThread.isCurrent() == false) {
			Stamina::threadInvoke(Konnekt::mainThread, boost::bind(sendPostponed));
			return;
		}
		Stamina::TimerDynamic* timer = Stamina::timerTmplCreate( boost::bind(&send, false), true );
		timer->start((minimumReportInterval + Stamina::random(30, 60) ) * 1000);
	}

	// rozpoczyna proby wyslania danych na serwer.
	void send(bool force) {
		if (!running || statSemaphore.isOpened() == false) return;
		if (Connections::isConnected() == false) {
			sendPostponed();
			return;
		}
		Konnekt::threadRunner->run(boost::bind(&Beta::sendThread, force), "Beta::send");
	}

	// dodaje raport do kolejki
	int report(int type , const char * title , const char * msg , const char * digest, const CStdString& log, bool send) {
		Tables::oTableImpl reports(tableReports);
		tRowId i = reports->addRow();
		reports->setInt64(i , REP_DATE , _time64(0));
		reports->setInt(i , REP_TYPE , type);
		reports->setStr(i , REP_TITLE , title);
		reports->setStr(i , REP_MSG , msg);
		reports->setStr(i , REP_OS , info_os());
		reports->setStr(i , REP_PLUGS , info_plugins());
		reports->setInt(i , REP_VERSION , Konnekt::versionNumber);
		reports->setStr(i , REP_INFO , info_other());
		reports->setStr(i , REP_LOG , log);
		reports->setStr(i , REP_DIGEST , digest);
		reports->save();
		if (send != 0) Beta::send(true);
		return i;
	}

	string makeDigest(const string& txt) {
		__int64 digest = MD5_64(txt.c_str());
		
		return inttostr(digest&0xFFFFFFFF, 16, 8) + inttostr((digest >> 32) & 0xFFFFFFFF, 16, 8);
	}


	void cutoffWindow(HWND hwnd) {
		/*
		CStdString buff1 , buff2;
		GetWindowText(hwnd , buff1.GetBuffer(100) , 100);
		buff1.ReleaseBuffer();
		GetClassName(hwnd , buff2.GetBuffer(100) , 100);
		buff2.ReleaseBuffer();
*/
		// oglupiamy
		SetWindowLong(hwnd , GWL_WNDPROC , (LONG)DummyProc);
//		EnableWindow(hwnd , false); //Wylaczamy
		ShowWindowAsync(hwnd , SW_HIDE); // Chowamy
	}

	void lockdown() {
		// Zatrzymujemy wszystkie watki oprocz aktywnego...
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		unsigned int current = GetCurrentThreadId();
		HANDLE currentHandle = GetCurrentThread();
		for (tThreads::iterator it=threads.begin(); it!=threads.end(); it++) {
			unsigned int onlist = it->first;
			HANDLE handle = it->second;
			if (onlist != current && handle != currentHandle) {
				SuspendThread(handle);
			}
		}
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

		// Trzeba zatrzymaæ wszystkie w¹tki i zablokowac/schowac wszystkie okna...
		HWND hwnd = GetWindow(GetDesktopWindow(),GW_CHILD);

		statTimer.stop();

		running = false;
		while ((hwnd=GetNextWindow(hwnd,GW_HWNDNEXT))!=NULL) {
			DWORD procID;
			GetWindowThreadProcessId(hwnd , &procID);
			if (procID == GetCurrentProcessId()) {
				cutoffWindow(hwnd);
			}
		}


	}

	void errorreport(int type , const CStdString & title , CStdString & msg, const char * digest, const CStdString& log) {
		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n\n%s\n--------------------\n%s\n\n" , title.c_str() , msg.c_str());
		}

		int id = report(type , title , msg , digest, log, false); // Zapisujemy to, co juz znamy
		if (Debug::logFile) {
			fflush(Debug::logFile);
		}

		ErrorReport ER;
		CStdString format;
		LoadString(Stamina::getHInstance() , IDS_ERR_EXCEPTION , format.GetBuffer(500) , 500);
		format.ReleaseBuffer();
		stringf(ER.msg , format , msg.c_str());
		ER.msg = RegEx::doReplace("/(?<!\\r)\\n/g" , "\r\n" , ER.msg);
		DialogBoxParam(Stamina::getHInstance() , MAKEINTRESOURCE(IDD_ERROR) , 0 , ErrorDialogProc , (LPARAM)&ER);   
		// To co sie zmienilo, trzeba teraz zapisac
		if (ER.info.empty()) return;
		msg += "\n-----------------------------------\nINFO:  ";
		msg += ER.info;
		oTableImpl reports(tableReports);
		reports->setStr(id , REP_MSG , msg);
		reports->save();
		IMLOG("- reports saved");
	}

	void  stackdigest(string & msg) {
		msg+="\nStack trace:\n";
		msg+=Debug::stackTrace;
		msg+="\n";
	}
	void imdigest(string & msg) {
#ifdef __DEBUG
		// Wypisuje informacje o aktualnym/ostatnim IMessage
		if (!running || !TLSU().lastIM.inMessage) {
			msg+=stringf("\n%sIM: %d(0x%x , 0x%x)(%dB) [%s->%s]\n" , TLSU().lastIM.inMessage?"in":"last" , TLSU().lastIM.debug.id , TLSU().lastIM.debug.p1 , TLSU().lastIM.debug.p2 , TLSU().lastIM.debug.size , Plug.Name(TLSU().lastIM.debug.sender).c_str() , Plug.Name(TLSU().lastIM.debug.rcvr));
		} else {
			Debug::sIMDebug IMD;
			Debug::IMDebug_transform(IMD , TLSU().lastIM.msg , 0 , 0);
			msg+=stringf("\ninIM[%d]: %s(%.50s | %.50s)(%dB) [%s->%s]\n" , TLSU().lastIM.inMessage , IMD.id.c_str() , IMD.p1.c_str() , IMD.p2.c_str() , TLSU().lastIM.debug.size , IMD.sender.c_str() , Plug.Name(TLSU().lastIM.debug.rcvr).c_str());

		}
#endif
	}


};};
