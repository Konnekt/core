//#ifdef __BETA

#include "stdafx.h"
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

#include <Stamina\Timer.h>
#include <Stamina\Internet.h>
#include <Stamina\Time64.h>
#include <Stamina\RegEx.h>
#include <Stamina\Helpers.h>

#define URL_BETA_CENTRAL "http://beta%d.konnekt.info/beta_central.php"
//#define URL_BETA_CENTRAL "http://test.konnekt.info/beta/beta_central.php"

using Stamina::RegEx;

using Stamina::ExceptionString;

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



	CdTable Beta , Reports , Stats;
	DWORD time_started;
	__time64_t lastStat;


	void init() {
		CdtFileBin FBin;
		Tables::savePrepare(FBin);
		time_started = GetTickCount();
		lastStat = _time64(0);
		Beta.cols.deftype = DT_CT_INT;
		Reports.cols.deftype = DT_CT_PCHAR | DT_CF_CXOR;
		Stats.cols.deftype = DT_CT_INT;
		Beta.cxor_key = XOR_KEY;
		Reports.cxor_key = XOR_KEY;
		Stats.cxor_key = XOR_KEY;
		// Beta.cols.setcolcount(10 , DT_CC_FILL);
		Reports.cols.setcolcount(9 , DT_CC_FILL);

		Beta.cols.setcol(BETA_LOGIN , DT_CT_PCHAR|DT_CF_CXOR , (void*)"");
		Beta.cols.setcol(BETA_PASSMD5 , DT_CT_PCHAR|DT_CF_CXOR , (void*)"");
		Beta.cols.setcol(BETA_AFIREWALL , DT_CT_INT , 0);
		Beta.cols.setcol(BETA_AMODEM , DT_CT_INT , 0);
		Beta.cols.setcol(BETA_AWBLINDS , DT_CT_INT , 0);
		Beta.cols.setcol(BETA_ANONYMOUS_LOGIN , DT_CT_PCHAR|DT_CF_CXOR , (void*)"");
		Beta.cols.setcol(BETA_ANONYMOUS , DT_CT_INT , (void*)1);
		Beta.cols.setcol(BETA_LAST_REPORT , DT_CT_64 , 0);
		Beta.cols.setcol(BETA_URID , DT_CT_INT , 0);
		Beta.cols.setcol(BETA_LOGIN_CHANGE, DT_CT_64, 0);
		Beta.cols.setcol(BETA_LAST_STATICREPORT, DT_CT_64, 0);

		Stats.cols.setcol(STATS_DATE , DT_CT_64 , 0);
		Stats.cols.setcol(STATS_STARTCOUNT , DT_CT_INT , 0);
		Stats.cols.setcol(STATS_UPTIME , DT_CT_INT , 0);
		Stats.cols.setcol(STATS_MSGRECV , DT_CT_INT , 0);
		Stats.cols.setcol(STATS_MSGSENT , DT_CT_INT , 0);
		Stats.cols.setcol(STATS_REPORTED , DT_CT_INT , 0);

		Reports.cols.setcol(REP_DATE , DT_CT_64);
		Reports.cols.setcol(REP_TYPE , DT_CT_INT);
		Reports.cols.setcol(REP_TITLE , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_MSG , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_OS , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_PLUGS , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_INFO , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_LOG , DT_CT_PCHAR | DT_CT_CXOR);
		Reports.cols.setcol(REP_REPORTED , DT_CT_INT , 0);
		Reports.cols.setcol(REP_DIGEST , DT_CT_PCHAR , 0);
		Reports.cols.setcol(REP_VERSION , DT_CT_INT);

		FBin.assign(&Beta);
		FBin.load(FILE_BETA);
		if (!Beta.getrowcount()) Beta.addrow();
		// Beta.setint(0 , BETA_STARTCOUNT , Beta.getint(0 , BETA_STARTCOUNT) + 1);
		// FBin.save(FILE_BETA);

		setStats(0);

		if (!*Beta.getch(0 , BETA_LOGIN) && !Beta.getint(0 , BETA_ANONYMOUS)) showBeta();
		betaLogin = Beta.getch(0, BETA_LOGIN);
		betaPass = Beta.getch(0, BETA_PASSMD5);
		anonymous = Beta.getint(0 , BETA_ANONYMOUS);
		if (!betaLogin.empty() && betaLogin[0]!=ANONYM_CHAR) {anonymous = false;} // Jawna nieprawid³owoœæ :)
	}

	void showBeta(bool modal) {
		if (!hwndBeta) {
			hwndBeta=1;
			DialogBox(hInst , MAKEINTRESOURCE(IDD_BETA)
				, 0 , (DLGPROC)BetaDialogProc);
		}               
		//  else SetForegroundWindow(hwndBeta);
	}
	void showReport() {
		if (anonymous) return;
		ShowWindow(CreateDialog(hInst , MAKEINTRESOURCE(IDD_REPORT)
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
		return; // DISABLE stats
		
		Stamina::SemaphoreCtx sctx (statSemaphore);

		CdtFileBin FBin;
		Tables::savePrepare(FBin);
		static bool firstStat = true;
		static int lastMsgSent = 0;
		static int lastMsgRecv = 0;
		if (forTime == 0)
			forTime = lastStat;
		Stats.lock(-1);
		FBin.assign(&Stats);
		FBin.load(FILE_STATS);
		// Odnajdujemy/tworzymy aktualn¹ datê...
		// 86400 sek/dzieñ
		int days = (int)(forTime / 86400);
		int id = DT_MASKID(days);
		if (DT_UNMASKID(id) > DT_COLID_UNIQUE) {
			Stats.unlock(-1);
			return; // coœ siê nie zgadza!!!
		}
		if (Stats.getrowpos(id) == -1)
			Stats.addrow(id);
		int row = Stats.getrowpos(id); // ¿eby by³o szybciej...
		if (firstStat) {
			Stats.setint(row , STATS_STARTCOUNT , Stats.getint(row , STATS_STARTCOUNT) + 1);
		}
		int uptime;
		uptime = (GetTickCount() - time_started) / 1000;
		if (uptime > 86400)
			uptime = 86400;
		if (uptime > 0)
			Stats.setint(row , STATS_UPTIME , Stats.getint(row , STATS_UPTIME) + uptime);
		Stats.setint(row , STATS_MSGSENT , Stats.getint(row , STATS_MSGSENT) + Messages::msgSent);
		Stats.setint(row , STATS_MSGRECV , Stats.getint(row , STATS_MSGRECV) + Messages::msgRecv);
		Stats.set64(row , STATS_DATE , (days * 86400) + 14400 );
		// 

		// Zerujemy wszystkie liczniki
		time_started = GetTickCount();
		lastStat = _time64(0);
		lastMsgRecv = Messages::msgRecv;
		lastMsgSent = Messages::msgSent;
		// Zamawiamy timer, do wykonania nastêpnego dnia...
		statTimer.start(setStats, 0, (86400 - (lastStat % 86400)) * 1000);
		IMLOG("Statystyka id = %d; uptime = %d; msgSent = %d; msgRecv = %d" , id , uptime , Messages::msgSent , Messages::msgRecv);

		FBin.save(FILE_STATS);
		IMLOG("- Beta report saved");
		firstStat = false;
		Stats.unlock(-1);
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
			s += inttostr(Plug[i].versionNumber,16);
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
		str+=inttoch(ovi.dwMajorVersion);
		str+=".";
		str+=inttoch(ovi.dwMinorVersion);
		str+=".";
		str+=inttoch(ovi.dwPlatformId==VER_PLATFORM_WIN32_NT?ovi.dwBuildNumber:LOWORD(ovi.dwBuildNumber));
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
			str+=inttoch(mhz);
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

	// Zwraca "numer seryjny" kopii Konnekta na podstawie kilku charakterystycznych wartoœci komputera i œcie¿ki
	string info_serial() {
		DWORD serialNumberSystem = 0;
		DWORD serialNumberInstance = 0;
		__int64 serialSystem = 0;
		__int64 serialInstance = 0;
		if (GetVolumeInformation("C:\\" , 0 , 0 , &serialNumberSystem , 0 , 0 , 0 , 0)) {
			serialSystem = MD5_64(inttostr(serialNumberSystem));
		} else {
			serialSystem = 0;
		}
		CStdString instancePath = appPath;
		instancePath = Stamina::RegEx::doGet("#^((?:[a-z]:\\\\)|(?:\\\\{2}.+?\\\\.+?\\\\))#i", instancePath, 1, instancePath);
		if (GetVolumeInformation(instancePath , 0 , 0 , &serialNumberInstance , 0 , 0 , 0 , 0)) {
			serialInstance = MD5_64(inttostr(serialNumberInstance) + appPath);
		} else {
			serialInstance = serialSystem;
		}
		CStdString serial;
		serial.Format("%16I64X%16I64X", serialSystem, serialInstance);
		//return serialSystem.substr(0, 16) + serialInstance.substr(0, 16);
		return serial;
	}

	string info_log(){
		string s = "";
#ifdef __DEBUG
		if (Debug::logFile) {
			FILE * f = fopen(Debug::logFileName , "rt");
			if (!f) return "";
			fseek(f , -logReadout , SEEK_END);
			memset(TLS().buff , 0 , logReadout + 1);
			if (fread(TLS().buff , 1 , logReadout , f)) {
				s = "..."+string(TLS().buff);
			}
			fclose(f);
		}
#endif
		return s;
	}


	unsigned int sendThread(bool force) {
		return 0; // DISABLE sending
		
		Stamina::randomSeed();

		Stamina::SemaphoreCtx sctx (sendSemaphore);

		CdtFileBin FBin;
		Tables::savePrepare(FBin);
		FBin.assign(&Beta);
		FBin.load(FILE_BETA);
		if (!Beta.getrowcount()) Beta.addrow();

		// Jakiœ BETA_URID musi byæ ustawiony...
		if (Beta.getint(0 , BETA_URID) == 0) {
			int newUrid = (_time64(0) - reportInterval) & 0xFFFFFF;
			Beta.setint(0, BETA_URID, newUrid);
			// od razu tworzymy katalog
			if (_access(PATH_BETA, 0)) {
				mkdir(PATH_BETA);
			}
			FBin.assign(&Beta);
			FBin.save(FILE_BETA);
			Beta.clearrows();
			FBin.load(FILE_BETA);
			if (Beta.getint(0, BETA_URID) != newUrid) {
				K_ASSERT_MSG(0, "Z³y BETA_URID!");
				IMDEBUG(DBG_ERROR, "- [BT]Problem with beta.dtb");
				return 0;
			}
		}


		FBin.assign(&Reports);
		FBin.load(FILE_REPORTS);
		bool retry=false;
		bool isLocked = false;
		// Gdy nie robimy Resend, nie ma raportów i nie minê³o wystarczaj¹co du¿o czasu, olewamy sprawê
		int interval = abs((int)(_time64(0) - Beta.get64(0 , BETA_LAST_REPORT)));

		bool anonymous = Beta.getint(0, BETA_ANONYMOUS);
		int currentMinimumInterval = anonymous ? anonymousMinimumReportInterval : minimumReportInterval;
		int currentInterval = anonymous ? anonymousReportInterval : reportInterval;

		if (!force) {
            // szukamy nowszych raportów
			bool gotNewReports = false;
			__int64 lastReport = Beta.get64(0 , BETA_LAST_REPORT);
			for (unsigned int i=0; i<Reports.getrowcount(); i++) {
				if (Reports.get64(i, REP_DATE) > lastReport)
					gotNewReports = true;
			}

			if ( (interval < currentMinimumInterval) || ( (interval < currentInterval) && gotNewReports == false ) ) {
				IMLOG("- [BT]Nothing to report, interval = %d", interval);
				return 0;
			}

		}
		sendRetries++;

		FBin.assign(&Stats);
		FBin.load(FILE_STATS);

		//   IMLOG("- Beta comm %s" , lParam?"retry":"first try");
		FILE * log;
		try {
			log = fopen(Debug::logPath + "reports.log" , "at");


			Beta.lock(-1);
			Reports.lock(-1);
			Stats.lock(-1);
			isLocked = true;		

			IMLOG("* BT Zbieram dane...");
			string frmdata;
			// login , pass		
			frmdata = "login="+urlEncode(Beta.getch(0 , BETA_LOGIN))
				+"&pass="+urlEncode(Beta.getch(0 , BETA_PASSMD5));
			// urid (Unique Report ID) - do wykrywania powtórnych wys³añ
			int urid = Beta.getint(0 , BETA_URID);
			frmdata += "&urid=" + inttostr(urid, 16);
			// anonymous
			if (*Beta.getch(0 , BETA_ANONYMOUS_LOGIN)) frmdata+="&anonymous="+urlEncode(Beta.getch(0 , BETA_ANONYMOUS_LOGIN));
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

			int reportsCount = 0;
			int statsCount = 0;

			if (Beta.getch(0 , BETA_LOGIN) || Beta.getch(0, BETA_LAST_REPORT)) {
				// Statystyki
				for (unsigned int i=0; i<Stats.getrowcount(); i++) {
					if ((int)(Stats.get64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
					if (Stats.getint(i, STATS_REPORTED)) {
						continue;
					}
					statsCount ++;
					string var = "&S[";
					var += inttoch(i);
					var += "]";
					frmdata += var + "[id]=" + inttoch(Stats.getrowid(i), 16);
					frmdata += var + "[date]=" + cTime64(Stats.get64(i , STATS_DATE)).strftime("%Y-%m-%d"); 
					frmdata += var + "[uptime]="+inttoch(Stats.getint(i , STATS_UPTIME));
					frmdata += var + "[count]="+inttoch(Stats.getint(i , STATS_STARTCOUNT));
					frmdata += var + "[msent]="+inttoch(Stats.getint(i , STATS_MSGSENT));
					frmdata += var + "[mrecv]="+inttoch(Stats.getint(i , STATS_MSGRECV));
				}
				// Raporty
				for (unsigned int i=0; i<Reports.getrowcount(); i++) {
					if (Reports.getint(i, REP_REPORTED)) {
						continue;
					}
					reportsCount ++;
					string var = "&R[";
					var += inttoch(i);
					var += "]";
					frmdata += var + "[id]=" + inttoch(Reports.getrowid(i), 16);
					frmdata += var + "[date]=" + cTime64(Reports.get64(i , REP_DATE)).strftime("%Y-%m-%d"); 
					frmdata += var + "[type]="+inttoch(Reports.getint(i , REP_TYPE));
					frmdata += var + "[title]="+urlEncode(Reports.getch(i , REP_TITLE));
					frmdata += var + "[msg]="+urlEncode(Reports.getch(i , REP_MSG));
					frmdata += var + "[os]="+urlEncode(Reports.getch(i , REP_OS));
					frmdata += var + "[nfo]="+urlEncode(Reports.getch(i , REP_INFO));
					CStdString log = Reports.getch(i , REP_LOG);
					if (log.length() > logSendout) {
						log.erase(0, log.length() - logSendout);
					}
					frmdata += var + "[log]="+urlEncode(log);
					frmdata += var + "[digest]="+urlEncode(Reports.getch(i , REP_DIGEST));
					frmdata += var + "[version]="+(std::string)inttostr(Reports.getint(i , REP_VERSION), 16);
					frmdata += var + "[plugins]="+urlEncode(Reports.getch(i , REP_PLUGS));
				}
			} // have login...
			Beta.unlock(-1);
			Reports.unlock(-1);
			Stats.unlock(-1);
			isLocked = false;

			IMLOG("* BT Próbujê po³¹czyæ...");

			CStdString url;
			srand(time(0));
			url.Format(URL_BETA_CENTRAL, Stamina::random(1, BETA_SERVER_COUNT) );
			url += "?";
			if (Beta.getch(0 , BETA_LOGIN)[0] == 0) {
				url += "nologin=1";
			} else if (Beta.getint(0 , BETA_ANONYMOUS)){
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
			if (Beta.get64(0, BETA_LOGIN_CHANGE) > Beta.get64(0, BETA_LAST_REPORT)) {
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

			Beta.lock(-1);
			Reports.lock(-1);
			Stats.lock(-1);
			isLocked = true;		

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
				Beta.setch(0 , BETA_LOGIN , "");
				Beta.setch(0 , BETA_PASSMD5 , "");
				Beta.setint(0 , BETA_ANONYMOUS , 0);

				betaLogin = "";
				betaPass = "";
				anonymous = true;

				FBin.assign(&Beta);
				FBin.save(FILE_BETA);
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
				//deInit();
				//loggedIn = false;
			} else {
				//loggedIn = true;
			}
			loggedIn = true; // zawsze zalogowany...

			if (loggedIn) {
				response.reset();
				// User zosta³ zarejestrowany jako anonim...
				if (response.match("/^Registered=\"(.+),(.*)\"$/m")) {
					Beta.setch(0 , BETA_LOGIN , response[1].c_str());
					Beta.setch(0 , BETA_PASSMD5 , response[2].c_str());
					Beta.setint(0 , BETA_ANONYMOUS , 1);
					betaLogin = response[1];
					betaPass = response[2];
					anonymous = true;
				}
				// User ju¿ nie jest anonimem...
				if (response.match("/^Unregistered$/m")) {
					Beta.setch(0 , BETA_ANONYMOUS_LOGIN , "");
				}
				if (response.match("/^Reported$/m")) {
					Beta.set64(0 , BETA_LAST_REPORT , _time64(0));
					// ¯eby za ka¿dym razem sprawdza³ czy mo¿e pisaæ po beta.dtb
					//Beta.setint(0 , BETA_URID , _time64(0) & 0xFFFFFF);
					Beta.setint(0 , BETA_URID , 0);
				}
				if (response.match("/^StatsStoredLast (\\d+)$/m")) {
					__int64 last = (int)(_atoi64(response[1].c_str()) / 86400) * 86400;
					for (unsigned int i=0; i<Stats.getrowcount(); i++) {
						if ((int)(Stats.get64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)
							|| Stats.get64(i , STATS_DATE) > last
							) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
						Stats.deleterow(i);
						i--;
					}
				} else if (response.match("/^StatsStored$/m")) {
					for (unsigned int i=0; i<Stats.getrowcount(); i++) {
						if ((int)(Stats.get64(i , STATS_DATE)/86400) == (int)(_time64(0)/86400)) continue; // Pomijamy niepe³ne, dzisiejsze statystyki...
						Stats.deleterow(i);
						i--;
					}
				}
				if (response.match("/^ReportsStoredLast (\\d+)$/m")) {
					__int64 last = _atoi64(response[1].c_str());
					for (unsigned int i=0; i<Reports.getrowcount(); i++) {
						if (Reports.get64(i , REP_DATE) > last) continue; // pomijamy
						//Reports.deleterow(i);
						//i--;
						Reports.setint(i, REP_REPORTED, 1);
					}
				} else if (response.match("/^ReportsStored$/m")) {
//					Reports.clearrows();
					for (unsigned int i=0; i<Reports.getrowcount(); i++) {
						Reports.setint(i, REP_REPORTED, 1);
					}

				}
				if (response.match("/^Retry=(\\d+)$/m")) {
					retry = true;
				}
			} // loggedIn
		} catch (Stamina::ExceptionInternet e)	{
			IMLOG("![BT] Wyst¹pi³ b³¹d sieci GLE:%d WSA:%d [%s]!" , GetLastError() , WSAGetLastError(), e.getReason().c_str());
			retry=true; // mo¿emy spróbowaæ za jakiœ czas skoro teraz s¹ byæ mo¿e problemy z sieci¹...
		} catch (ExceptionString e)	{
			IMLOG("![BT] Wyst¹pi³ b³¹d GLE:%d [%s]!" , GetLastError() , e.getReason().c_str());
			retry=false; // skoro dostaliœmy jak¹œ odpowiedŸ i by³a uszkodzona - nie pytamy na razie wiêcej!
			// i ustawiamy ¿eby nie pyta³ przez najbli¿szy czas równie¿...
			Beta.set64(0 , BETA_LAST_REPORT , _time64(0));
			// 
			//Beta.setint(0 , BETA_URID , _time64(0) & 0xFFFFFF);
			Beta.setint(0 , BETA_URID , 0);
		}

		FBin.assign(&Beta);
		FBin.save(FILE_BETA);
		FBin.assign(&Reports);
		FBin.save(FILE_REPORTS);
		FBin.assign(&Stats);
		FBin.save(FILE_STATS);

		//  IMLOG("finish");
		if (log)
			fclose(log);
		if (isLocked) {
			Reports.unlock(-1);
			Beta.unlock(-1);
			Stats.unlock(-1);
		}
		if (retry) {
			sendPostponed();
		}
		return 0;
	}


	void sendPostponed() {
		return; // DISABLE SENDING
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
		return; // DISABLE SENDING
		if (!running || statSemaphore.isOpened() == false) return;
		if (Connections::isConnected() == false) {
			sendPostponed();
			return;
		}
		Konnekt::threadRunner->run(boost::bind(&Beta::sendThread, force), "Beta::send");
	}

	// dodaje raport do kolejki
	int report(int type , const char * title , const char * msg , const char * digest, const CStdString& log, bool send) {
		CdtFileBin FBin;
		Tables::savePrepare(FBin);
		FBin.assign(&Reports);
		FBin.load(FILE_REPORTS);
		int i = Reports.addrow();
		Reports.set64(i , REP_DATE , _time64(0));
		Reports.setint(i , REP_TYPE , type);
		Reports.setch(i , REP_TITLE , title);
		Reports.setch(i , REP_MSG , msg);
		Reports.setch(i , REP_OS , info_os().c_str());
		Reports.setch(i , REP_PLUGS , info_plugins().c_str());
		Reports.setint(i , REP_VERSION , Konnekt::versionNumber);
		Reports.setch(i , REP_INFO , info_other().c_str());
		Reports.setch(i , REP_LOG , log);
		Reports.setch(i , REP_DIGEST , digest);
		FBin.assign(&Reports);
		FBin.save(FILE_REPORTS);
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
		LoadString(hInst , IDS_ERR_EXCEPTION , format.GetBuffer(500) , 500);
		format.ReleaseBuffer();
		stringf(ER.msg , format , msg.c_str());
		ER.msg = RegEx::doReplace("/(?<!\\r)\\n/g" , "\r\n" , ER.msg);
		DialogBoxParam(hInst , MAKEINTRESOURCE(IDD_ERROR) , 0 , ErrorDialogProc , (LPARAM)&ER);   
		// To co sie zmienilo, trzeba teraz zapisac
		if (ER.info.empty()) return;
		msg+="\n-----------------------------------\nINFO:  ";
		msg+=ER.info;
		Reports.setch(id , REP_MSG , msg);
		CdtFileBin FBin;
		FBin.assign(&Reports);
		FBin.save(FILE_REPORTS);
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
			msg+=stringf("\n%sIM: %d(0x%x , 0x%x)(%dB) [%s->%s]\n" , TLSU().lastIM.inMessage?"in":"last" , TLSU().lastIM.debug.id , TLSU().lastIM.debug.p1 , TLSU().lastIM.debug.p2 , TLSU().lastIM.debug.size , Plug.Name(TLSU().lastIM.debug.sender) , Plug.Name(TLSU().lastIM.debug.rcvr));
		} else {
			Debug::sIMDebug IMD;
			Debug::IMDebug_transform(IMD , TLSU().lastIM.msg , 0 , 0);
			msg+=stringf("\ninIM[%d]: %s(%.50s | %.50s)(%dB) [%s->%s]\n" , TLSU().lastIM.inMessage , IMD.id.c_str() , IMD.p1.c_str() , IMD.p2.c_str() , TLSU().lastIM.debug.size , IMD.sender.c_str() , Plug.Name(TLSU().lastIM.debug.rcvr));

		}
#endif
	}


};};
