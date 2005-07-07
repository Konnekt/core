#pragma once

#include "stdafx.h"

#include "main.h"
#include "resources.h"
#include "konnekt_sdk.h"
#include "beta.h"
#include "debug.h"
#include "plugins.h"
#include "plugins_ctrl.h"
#include "imessage.h"
#include "profiles.h"
#include "tables.h"
#include "connections.h"
#include "contacts.h"
#include "messages.h"
#include "windows.h"
#include "error.h"
#include "unique.h"
#include "argv.h"

#include <Stamina\Logger.h>
#include <Stamina\Exception.h>
#include "KLogger.h"


#pragma comment(lib,"Ws2_32.lib")

IMPLEMENT_PURECALL_EXCEPTION("Konnekt.exe");

namespace Konnekt {
	using namespace Debug;

	CStdString appPath;
	CStdString exePath;
	CStdString app;
	CStdString sessionName; // ID sesji - MD5 katalogu z profilem
	CStdString tempPath;
	CStdString dataPath;
	bool isComCtl6 = false;
	HINTERNET hInternet = 0; // uchwyt do globalnej sesji sieciowej...
	bool noCatch = false;
	//	bool suspended = false;

	CStdString versionInfo;
	unsigned int versionNumber;
	CStdString  suiteVersionInfo;
	bool newVersion = true;

	HANDLE hMainThread;
	unsigned long MainThreadID; 
	unsigned int ui_sender;
	bool canQuit = true;
	bool running = false;
	bool isRunning = true;
	bool isWaiting = false;
	bool canWait = true; // Czy mo¿na czekaæ na cokolwiek (overlapped IO itd...)
	HANDLE semaphore;



	void WMessages(bool loop) {
		MSG msg;
		HWND fhWnd=0;
		static HANDLE nullEvent = CreateEvent(0, 0, 0, 0);
		while (1) {
			//              if (!isComCtl6) SleepEx(0,1);
			if (canWait) 
				/* Zatkany message loop blokuje funkcje APC
				, jednorazowo przegladamy wiec kolejke. nullEvent nigdy nie bedzie zasygnalizowany... */
				WaitForSingleObjectEx(nullEvent, 0, TRUE);
			if (!loop) {
				if (!PeekMessage(&msg , 0 , 0 , 0 , PM_REMOVE)) return;
			} else {
				//                 if (Debug::logFile) fprintf(Debug::logFile , ".");fflush(Debug::logFile);
				//                 debugLogPut(".");
				/*if (!PeekMessage(&msg, NULL, 0, 0 , PM_REMOVE))*/ {
				isWaiting=true;
				int wait_ret=MsgWaitForMultipleObjectsEx(0 , 0 , INFINITE , QS_ALLINPUT | QS_ALLPOSTMESSAGE , (canWait ? MWMO_ALERTABLE : 0) | MWMO_INPUTAVAILABLE);
				//                 if (Debug::logFile) fprintf(Debug::logFile , "%d",wait_ret);fflush(Debug::logFile);
				//                 if (wait_ret==WAIT_FAILED) {fprintf(Debug::logFile , "\nWAIT FAILED! %d\n" , GetLastError());}
				//                 SleepEx(0,1);
				isWaiting=false;
				if (wait_ret == MWMO_ALERTABLE || !PeekMessage(&msg, NULL, 0, 0 , PM_REMOVE))
					continue;
				}
				//                 {fprintf(Debug::logFile,"!");continue;}
				//                 else fprintf(Debug::logFile,".");
			}
			if (!msg.hwnd && (msg.message == WM_QUIT)) {
				if (!loop) { IMLOG("- Received WM_QUIT outside of loop");deInit();return;}
				IMLOG("- Received WM_QUIT!"); return;
			}
			if (msg.message == WM_TIMECHANGE) {
				Beta::setStats(Beta::lastStat);
			}
			else if (msg.message == WM_ENDSESSION) {
				IMLOG("- WM_ENDSESSION caught, continue = %d" , !running);
				if (!running)
					continue;
				else
					deInit(true);

			}
			//          if (!msg.hwnd)  {IMLOG("________THREADMSG [%d]!" , msg.message);}
			//          if (msg.message >= MYWM_CORE && msg.message <= MYWM_CORE + MYWM_CORECOUNT) {
			//            IMLOG("_____COREMSG! [%x]" , msg.hwnd);
			//            switch (msg.message) {
			//              case MYWM_MESSAGEQUEUE: CMessageQueue();
			//              case MYWM_SETCONNECT: CSetConnect(msg.lParam , msg.wParam);
			//              case MYWM_IM: ;
			//            }
			//            continue;
			//          }
			fhWnd = GetForegroundWindow();
			if (!fhWnd || !IsDialogMessage(fhWnd,&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			//else {}
		}
		//  Beep(400, 200);
	}

	void WinMainBlock(void) {
		Init();
		WMessages(1);
		deInit();
	}
	void WinMainSafeBlock(void) {
#if !defined(__NOCATCH) || defined(__PLUGCATCH)
		try {
#endif
		WinMainBlock();
#ifndef __NOCATCH
		}
		catch (int e) {
			//STACKTRACE();
			exception_information(inttoch(e,16,2));
			//break;//exit(0);
		}
		catch (const exception & e) {
			//STACKTRACE();
			exception_information(e.what());
			//break;//exit(0);
		}
		catch (char * e) {
			//STACKTRACE();
			exception_information(e);
			//break;
		}
		catch (string e) {
			//STACKTRACE();
			exception_information((char*)e.c_str());
			//break;
		}
#endif
#if !defined(__NOCATCH) || defined(__PLUGCATCH)
#if defined(__NOCATCH)
	}
#endif
	catch (exception_plug e) {
		int pos = Plug.FindID(e.id);              
		if (pos>=0) Plug[pos].madeError(e.msg , e.severity);
	}
	catch (exception_plug2 e) {
		int pos = Plug.FindID(e.id);              
		if (pos>=0) Plug[pos].madeError(e.msg , e.severity);
	}
	/*    #if !defined(__NOCATCH)
	catch (...) {
	exception_information("Unknown");
	}
	#endif */
#endif 
}

void WinMainSafeBlock2(void) {
#ifndef __NOCATCH
	__try { // C-style exceptions
#endif
		WinMainSafeBlock();
#ifndef __NOCATCH
	} 
	__except(except_filter((*GetExceptionInformation()),"Core")) 
	{
		IMLOG("Aborting...");
		exit(1);
	}
#endif // NOCATCH
}
/*HACK: Przenoszenie bibliotek z katalogu g³ównego...*/
void moveAdditionalLibraries() {
	WIN32_FIND_DATA fd;
	HANDLE hFile;
	if ((hFile = FindFirstFile(dataPath + "dll\\*.dll",&fd))!=INVALID_HANDLE_VALUE) {
		do {
			struct _stat stData, stMain;
			CStdString file = dataPath + "dll\\";
			file += fd.cFileName;
			CStdString fileMain = fd.cFileName;
			// je¿eli w kat. Konnekta nie ma tego dll'a, olewamy sprawê...
			// skoro ju¿ jesteœmy w tym miejscu, znaczy ¿e s¹ wszystkie potrzebne
			// do rozruchu.
			if (_access(fd.cFileName, 0)) continue;
			// niektóre DLLki po prostu musz¹ byæ w g³ównym...
			if (stristr(fd.cFileName, "msvc") == fd.cFileName) continue;
			if (!strcmp(fd.cFileName, "ui.dll")) continue;
			// koñczymy gdy jest katalogiem, lub nie mozemy odczytac czasow
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY 
				|| _stat(file , &stData)
				|| _stat(fileMain , &stMain))
				continue;
			if (stMain.st_mtime <= stData.st_mtime) {
				unlink(fileMain + ".old");
				rename(fileMain, fileMain + ".old");
			} else {
				unlink(file + ".old");
				rename(file, file + ".old");
			}
		} while (FindNextFile(hFile , &fd));
		FindClose(hFile);
	}

}


//------

void __cdecl atexit_finish ( void ){
	static bool once = false;
	if (once) return;
	once = true;

#ifdef __DEBUG
	if (Debug::logFile) {
		fprintf(Debug::logFile , "\n\nGracefull Exit\n");
		fclose(Debug::logFile);
		unlink(logPath + "konnekt_bck.log");
		rename(logPath + "konnekt.log" , logPath + "konnekt_bck.log");
		rename(logFileName , logPath + "konnekt.log");
	}
#endif
}

void gracefullExit() {
	atexit_finish();
	exit(0);
}


void __cdecl restart ( void ) {
	string args;
	if (!getArgV("-restart" , false)) {
		args += "-restart";
	}
	for (int i=1; i < _argc; i++) {
		if (!args.empty()) {
			args += " ";
		}
		args += "\"";
		args += _argv[i];
		args += "\"";
	}

#ifdef __DEBUG
	if (Debug::logFile) {
		fprintf(Debug::logFile , "\n\nRestarting: %s %s\n" , app.c_str() , args.c_str());
		fclose(Debug::logFile);
		Debug::logFile = 0;
	}
#endif

	atexit_finish();
	ShellExecute(0 , "open" , app , args.c_str() , "" , SW_SHOW);
	//   _execl(_argv[0] , _argv[0] , "/restart");
	gracefullExit();
}

void __cdecl remove ( void ) {
	rmdirtree((char*)profileDir.c_str());
	restart();
}
//---------------------------------------------------------------------------

int loadRegistry() {
	HKEY hKey=HKEY_CURRENT_USER;
	string str;
	if (!RegOpenKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,KEY_READ,&hKey))
	{
		//    if (!RegOpenKeyEx(hKey,"Stamina",0,KEY_READ,&hKey) &&
		//          !RegOpenKeyEx(hKey,sig.c_str(),0,KEY_READ,&hKey))
		//       if (!RegCreateKey(hKey,"Stamina"
		//      {
		if (getArgV(ARGV_PROFILE))
			profile = getArgV(ARGV_PROFILE , true);
		else
			profile=RegQuerySZ(hKey,"Profile");

		if (getArgV(ARGV_PROFILESDIR,true,0)) {
			profilesDir = getArgV(ARGV_PROFILESDIR , true);
		} else
			profilesDir=RegQuerySZ(hKey,"ProfilesDir");

		if (profilesDir.size() && profilesDir[profilesDir.size()-1]!='\\')
			profilesDir += '\\';
		str=RegQuerySZ(hKey,"Version");
		newVersion = (str != versionInfo);
		superUser=false;
#ifdef __DEBUG
		superUser=RegQueryDW(hKey , "superUser" , 0)==1;
		if (getArgV(ARGV_DEBUG))
			superUser = true;

		if (superUser) {
			//Debug::show=RegQueryDW(hKey , "dbg_show" , 1);
			Debug::x  = RegQueryDW(hKey , "dbg_x" , 0);
			Debug::y  = RegQueryDW(hKey , "dbg_y" , 0);
			Debug::w  = RegQueryDW(hKey , "dbg_w" , 0);
			Debug::h  = RegQueryDW(hKey , "dbg_h" , 0);
			//Debug::log = RegQueryDW(hKey , "dbg_log" , 0);
			//Debug::log = false;
			//Debug::logAll = RegQueryDW(hKey , "dbg_logAll" , 0);
			//Debug::logAll = false;
			Debug::scroll = RegQueryDW(hKey , "dbg_scroll" , 0);
		}   
#endif
		//      }
		RegCloseKey(hKey);
	}

	return 0;
}


int saveRegistry() {
	HKEY hKey=HKEY_CURRENT_USER , hKey2;
	if (
		//    !RegOpenKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,KEY_ALL_ACCESS,&hKey)
		!RegCreateKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,"",0,KEY_ALL_ACCESS,0,&hKey,0)
		)
	{
		if (!getArgV(ARGV_PROFILE))
			RegSetSZ(hKey,"Profile",profile);
		RegSetSZ(hKey,"ProfilesDir",profilesDir);
		RegSetSZ(hKey,"Version",versionInfo);
#ifdef __DEBUG
		
		if (superUser) {
			//RegSetDW(hKey,"dbg_show",IsWindowVisible(dbg.hwnd));
			RECT rc;
			GetWindowRect(Debug::hwnd , &rc);
			RegSetDW(hKey,"dbg_x",rc.left);
			RegSetDW(hKey,"dbg_y",rc.top);
			RegSetDW(hKey,"dbg_w",rc.right - rc.left);
			RegSetDW(hKey,"dbg_h",rc.bottom - rc.top);
			//RegSetDW(hKey,"dbg_log",dbg.log);
			//RegSetDW(hKey,"dbg_logAll",dbg.logAll);
			//dbg.logAll = false;
			RegSetDW(hKey,"dbg_scroll",Debug::scroll);
		}
		
#endif
		if (!RegCreateKeyEx(hKey,"DiscardPlugins",0,"",0,KEY_ALL_ACCESS,0,&hKey2,0))
		{
			RegCloseKey(hKey2);
		}
		RegCloseKey(hKey);
	}
	return 0;
}
bool RestoreRunning() {
	HWND wnd = FindWindow("UItop."+sessionName , 0);
	if (!wnd) {
		wnd = FindWindow("UImain" , 0); // niech bêdzie dowolny inny...
		if (wnd)
			wnd = GetParent(wnd);
	}
	if (!wnd) return false;
	/*	COPYDATASTRUCT cds;
	// trzeba przygotowaæ kopiê danych...
	cds.dwData = __argc;
	cds.cbData = (__argc + 1) * sizeof(int);
	// obliczamy rozmiar i wype³niamy offsety
	*/
	char ** data = new char* [__argc];//(int *)malloc(cds.cbData);
	int maxSize = __argc * sizeof(char*);
	for (int i=0; i < __argc; i++) {
		data[i] = (char*)maxSize;
		maxSize+=strlen(__argv[i])+1;
	}
	//	data[__argc] = (char*) maxSize;

	HANDLE fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE , 0 , PAGE_READWRITE , 0 , maxSize , "Local\\KONNEKT.File.RestoreData");
	char * base = (char *)MapViewOfFile(fileMapping , FILE_MAP_WRITE , 0 , 0 , maxSize);
	IMLOG("- restore - sending sz=%d" , maxSize);
	memcpy(base , data , __argc * sizeof(char*));
	for (int i=0; i < __argc; i++) {
		strcpy(base + (int)data[i] , __argv[i]);
		IMLOG("      +data %x=%s" , data[i] , base+(int)data[i]);
	}
	int r = SendMessage(wnd , RegisterWindowMessage("KONNEKT.Msg.RestoreData") , __argc , maxSize);
	//	cds.lpData = data;
	//	int r = SendMessage(wnd , WM_COPYDATA , 0 , (LPARAM)&cds);
	UnmapViewOfFile(base);
	CloseHandle(fileMapping);
	delete [] data;
	return r==TRUE;
}

void Init() {
	// dbg.showLog = true;

	Stamina::mainLogger = new Konnekt::KLogger(Stamina::logAll);


	srand( (unsigned)time( NULL ) );

#ifdef __BETA
	Beta::init();
#endif

	if (getArgV("-makeReport")) {
		CStdString value = getArgV("-makeReport", true, "");
		bool useDate = value.empty();
		bool silent = false;
		if (value == "all") {
			value = "";
		}
		if (value == "auto") {
			value = "";
			useDate = true;
			silent = true;
		}
		Beta::makeReport(value, true, useDate, silent);
		gracefullExit();
	}


	MainThreadID = GetCurrentThreadId();
	hMainThread = 0;
	DuplicateHandle(GetCurrentProcess() , GetCurrentThread()
		, GetCurrentProcess() , &hMainThread
		, 0 , 0 , DUPLICATE_SAME_ACCESS);

	// FileVersionInfo(_argv[0] , versionInfo);
	setlocale( LC_ALL, "Polish" );
	// dla restore
	if (getArgV(ARGV_RESTORE , false)) {
		RestoreRunning();
		gracefullExit();
	}


	WSADATA wsaData;
	WSAStartup( MAKEWORD( 2, 0 ), &wsaData );

	/*
	tm exp;
	memset(&exp , 0 , sizeof(tm));
	exp.tm_year = EXP_YEAR - 1900;
	exp.tm_mon = EXP_MONTH - 1;
	exp.tm_mday = EXP_DAY;
	if (EXP_YEAR && time(0)>mktime(&exp)) {
	string appName = resStr(IDS_APPNAME);
	MessageBox(NULL , resStr(IDS_ERR_EXP) , appName.c_str() ,MB_ICONERROR|MB_OK|MB_TASKMODAL );
	ShellExecute(0 , "open" , resStr(IDS_URL) , "" , "" , SW_SHOW);
	exit(0);
	}
	*/

	Unique::initializeDomains();

	memset(md5digest , 0 , 16);
	Plg.cxor_key = XOR_KEY;
	Msg.cxor_key = XOR_KEY;
	Cfg.cxor_key = XOR_KEY;
	Cnt.cxor_key = XOR_KEY;
	canQuit = false;
	// Ladowanie DLL'i

	if (Plug.PlugIN("ui.dll" , UI_CERT , 1)<1) {
		MessageBox(NULL ,resStr(IDS_ERR_NOUI),STRING(resStr(IDS_APPNAME)),MB_ICONERROR|MB_OK|MB_TASKMODAL ); 
		exit(0); 
	}
	// if (!PlugIN("gg.dll")) {MessageBox(NULL ,resStr(IDS_ERR_NONET),STR(resStr(IDS_APPNAME)),MB_ICONERROR|MB_OK|MB_TASKMODAL);exit(0); }
	IMLOG("--- UI loaded ---");
	if (profilesDir.empty()) setProfilesDir();
	IMLOG("--- ProfilesDir set ---");
	setPlgColumns();
	if (!setProfile(true , true)) 	gracefullExit();
	IMLOG("--- Profile set ---");
	IMLOG("profile=%s", profile.c_str());
	saveRegistry();

	// Sprawdzamy teraz, czy przypadkiem jakis inny K nie bawi sie juz tym profilem...
	GetFullPathName(profileDir.c_str() , 1024 , sessionName.GetBuffer(1024) , 0);
	IMLOG("profileDir=%s", profileDir.c_str());

	// ustawiamy œcie¿kê plików tymczasowych
	tempPath = (string(getenv("TEMP")) + "\\Konnekt_"+string(profile)+"_"+string(MD5Hex(CStdString(appPath + profileDir).ToLower())).substr(0, 8));
	mkdirs(tempPath);
	SetEnvironmentVariable("KonnektTemp" , tempPath);
	IMLOG("tempDir=%s", tempPath.c_str());


	sessionName.ReleaseBuffer();
	sessionName.ToLower();
	sessionName = MD5Hex((char*)sessionName.c_str()).substr(0 , 8);


	semaphore = CreateSemaphore(0 , 1 , 1 , "KONNEKT." + sessionName);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (!RestoreRunning())
			MessageBox(0 , STR(resStr(IDS_ERR_ONEINSTANCE)) , "Konnekt" , MB_OK | MB_ICONERROR);
		exit(0);
	}
	SetEnvironmentVariable("KonnektProfile" , profileDir.substr(0 , profileDir.size()-1).c_str());
	SetEnvironmentVariable("KonnektUser" , profile.c_str());

	// Ustawia kolumny dla Core
	setColumns();
	// Ustawia kolumny interfejsu
	IMessageDirect(Plug[0], IM_SETCOLS , 0 , 0 , 0);
	IMLOG("--- Core columns set ---");

	// £aduje pluginy...
	setPlugins();
	IMessage(IM_ALLPLUGINSINITIALIZED , NET_BROADCAST , IMT_ALL);
	IMLOG("--- Plugins loaded ---");

	// Ustawia inne wtyczki
	IMessage(IM_SETCOLS , NET_BROADCAST , IMT_ALL , 0 , 0 , 1);
	Msg.cols.optimize();
	Cfg.cols.optimize();
	Cnt.cols.optimize();
	IMLOG("--- Plugin columns set ---");
	loadProfile();
	Contacts::updateAllContacts();
	IMLOG("--- Profile loaded ---");

	// Wyœwietla okno ustalania "trudnoœci" (o ile zajdzie taka potrzeba...)
	IMessageDirect(Plug[0], IMI_SET_SHOWBITS , 0 , 0 , 0);

	hInternet = (HINTERNET) IMCore(&sIMessage_2params(IMC_HINTERNET_OPEN , (int)"Konnekt" , 0));

	checkVersions();
	IMLOG("--- Versions checked ---");
	Cfg.setch(0,CFG_APPFILE , app);
	Cfg.setch(0,CFG_APPDIR , appPath);
	IMessage(IM_UI_PREPARE,NET_BC,IMT_ALL,0,0 , 0);
	IMLOG("--- UI prepared ---");
	IMessage(IM_START ,NET_BC,IMT_ALL, 0, 0,1);
	IMessageDirect(Plug[0] , IM_START);
	IMLOG("--- Plugins started ---");
	if (newProfile) {
		IMessage(IM_NEW_PROFILE , NET_BC , IMT_ALL , 0 , 0 , 0);
		IMLOG("--- NEW profile passed ---");
	}

	Connections::startUp();
	IMessage(IM_NEEDCONNECTION,NET_BC,IMT_PROTOCOL,0,0 , 0);
	Connections::connect();
	IMLOG("--- Auto-Connected ---");
	Messages::initMessages();
	Messages::runMessageQueue(&sMESSAGESELECT(NET_BC , 0 , -1 , 0 , MF_SEND));
	IMLOG("--- First MSGQueue (rcv) ---");
	/*
	if (EXP_YEAR && (newVersion || newProfile)) {
	strftime(TLS().buff , MAX_STRING , "%#x" , &exp);
	string info = _sprintf(resStr(IDS_INF_EXP),versionInfo , TLS().buff);
	MessageBox(NULL , info.c_str() , STR(resStr(IDS_APPNAME)),MB_ICONWARNING|MB_OK );
	}
	*/
	// if (Cfg.getint(0 , CFG_AUTO_CONNECT))
	//   IMessage(IM_CONNECT , NET_BC , IMT_PROTOCOL);

	// Rozeslanie plug_args
	IMessageProcess(&sIMessage_plugArgs(__argc , __argv),0);

	IMLOG("---  ---  INITIALIZATION SUCCESSFULL  ---  ---");
	canQuit = true;
	running = true;
#ifdef __DEBUG
	Debug::showLog = true;
#endif
	// throw "sialalala";
	// char * s = 0;
	// s[5]='a';

#ifdef __BETA
	Beta::sendPostponed();
#endif


	return;
}


// ********************* deINIT

void end() {
	static bool once=0;
	if (!once) {
		once=true;
		IMessage(IM_DISCONNECT , NET_BC , IMT_PROTOCOL,0,0,0);
		IMessage(IM_END ,-1,IMT_ALL, !canWait , 0,1);
		IMessageDirect(Plug[0] , IM_END , !canWait , 0);
	}
}

void deInit(bool doExit) {
#ifdef __DEBUG
	Debug::showLog = false;
#endif
	static DWORD startTime=0;
	if (startTime != 0) {
		IMLOG("- deInit in progress for %d ms" , GetTickCount() - startTime);
		return;
	}
	IMLOG("- deInitialization started");
	startTime = GetTickCount();
	IMessage(IM_BEFOREEND , NET_BROADCAST , IMT_ALL , !canWait , 0);
	isRunning=false;
#ifdef __BETA
	Beta::close();
#endif
	// Beep(200,200);
	if (canQuit) {
		Tables::saveProfile();
		IMLOG("-profile saved");
		end();
		IMLOG("-IM_END broadcasted");
		Tables::saveProfile(false);
		// IMLOG("-profile saved if changed");
	}
	for (int i=1; i<Plug.size();i++) {
		Plug.PlugOUT(i);
	}
	Plug.PlugOUT(0);
	IMLOG("-plugs unpluged");
	Cnt.clearrows();
	Cnt.cols.clear();
	Cfg.clearrows();
	Cfg.cols.clear();
	Msg.clearrows();
	Msg.cols.clear();
	IMLOG("-tables cleaned");
#ifdef __DEBUG
	Debug::debug = false;
#endif
	//        #ifdef __BETA
	//          beta_close();
	//        #endif
	//IMLOG("Terminate");
	//        MessageBox(0,"","",0);
#ifdef __DEBUG
	fprintf(Debug::logFile , "\n______________________\n\nTerminate [%d ms]\n" , GetTickCount() - startTime);
	Debug::finish();
#endif
	//	return msg.wParam;
	delete Ctrl;
	CloseHandle(semaphore);
	InternetCloseHandle(hInternet);
	WSACleanup();
	if (doExit) {
		if (canQuit)
			gracefullExit();
		else
			exit(1);
	}
	return;
}

WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

#ifdef __DEBUGMEM
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | /*_CRTDBG_CHECK_ALWAYS_DF |*/ _CRTDBG_CHECK_EVERY_16_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	if (getArgV(ARGV_HELP)) {
		string nfo;
		nfo += "Konnekt.exe mo¿na uruchomiæ z nast. parametrami:\r\n";
		nfo += "\r\n  " ARGV_RESTORE " tylko przekazuje parametry do otwartego okna Konnekta";
		nfo += "\r\n  " ARGV_PROFILE "=<nazwa profilu lub ?> : uruchamia program z wybranym profilem (? - okno wyboru).";
		nfo += "\r\n  " ARGV_PROFILESDIR " : Otwiera aktualny katalog profili.";
		nfo += "\r\n  " ARGV_PROFILESDIR "=<nowy katalog> : ustawia nowy katalog z profilami. Pliki profili nale¿y przenieœæ rêcznie w razie potrzeby.";
		nfo += "\r\n  " ARGV_LOGPATH "=<œcie¿ka do katalogu> : zapisuje logi do wybranego katalogu.";
		nfo += "\r\n  " ARGV_PLUGINS " : wyœwietla okno ustawiania wtyczek.";
		nfo += "\r\n  " ARGV_OFFLINE " : wy³¹cza automatyczne ³¹czenie z sieci¹";
#ifdef __DEBUG
		nfo+= "\r\n  " ARGV_DEBUG " : uaktywnia menu do debugowania";
#endif
		MessageBox(0 , nfo.c_str() , "Konnekt.exe - parametry" , MB_ICONINFORMATION);
		gracefullExit();
	}


	isComCtl6 = isComctl(6,0);

	Ctrl = createCorePluginCtrl();
	Konnekt::CtrlEx = (cCtrlEx*)Ctrl;
	//appPath = _argv[0];
	GetModuleFileName(0 , exePath.GetBuffer(MAX_PATH) , MAX_PATH);
	exePath.ReleaseBuffer();
	_argv[0] = (char*)exePath.c_str();
	appPath = exePath;
	appPath.erase(appPath.find_last_of('\\')+1);
	if (!appPath.empty()) {
		_chdir(appPath.c_str());
	}
	_getcwd(appPath.GetBuffer(MAX_PATH) , MAX_PATH);
	appPath.ReleaseBuffer();
	if (appPath[appPath.size()-1]!='\\') appPath+='\\';
	dataPath = appPath + "data\\";

	CStdString envPath;
	GetEnvironmentVariable("PATH", envPath.GetBuffer(1024), 1024);
	envPath.ReleaseBuffer();
	SetEnvironmentVariable("PATH", dataPath + "dll;" + envPath);
	_SetDllDirectory(dataPath + "dll");

	if (_access(dataPath, 0)) {
		MessageBox(0, "Brakuje katalogu z danymi!\r\n" + dataPath, "Konnekt", MB_ICONERROR);
		gracefullExit();
	}




	/*HACK: Przenoszenie plików bety*/
	if (!_access("beta.dtb", 0) && _access(dataPath + "beta\\beta.dtb",0))  {
		mkdirs(dataPath + "beta");
		rename("beta.dtb", dataPath + "beta\\beta.dtb");
		rename("beta_reports.dtb", dataPath + "beta\\beta_reports.dtb");
		rename("beta_stats.dtb", dataPath + "beta\\beta_stats.dtb");
	}
	moveAdditionalLibraries();


	logPath = getArgV(ARGV_LOGPATH , true , dataPath+"log\\");
	if (logPath[logPath.size()-1]!='\\') logPath+='\\';
	app = _argv[0];
	app.erase(0 , app.find_last_of('\\')+1);
	app = appPath+app; 

	hInst = hInstance;
	{
		FILEVERSIONINFO fvi;
		FileVersionInfo (app , versionInfo.GetBuffer(100) , &fvi);
		versionInfo.ReleaseBuffer();
		versionNumber = VERSION_TO_NUM(fvi.major , fvi.minor , fvi.release , fvi.build);
		FileVersionInfo ("ui.dll" , suiteVersionInfo.GetBuffer(100));
		suiteVersionInfo.ReleaseBuffer();
		suiteVersionInfo = versionInfo + " | ui " + suiteVersionInfo;
	}
	loadRegistry();
	if (getArgV(ARGV_PROFILESDIR) && !getArgV(ARGV_PROFILESDIR , true , 0)) {
		if (profilesDir.empty()) {
			MessageBox(0 , "Katalog profili jeszcze nie zosta³ zdefiniowany!" , "Konnekt" , MB_ICONWARNING);
		} else {
			if (profilesDir[profilesDir.size()-1] == '\\') 
				profilesDir.erase(profilesDir.size()-1);
			MessageBox(0 , "Profile mieszcz¹ siê w katalogu: \n\"" + profilesDir +"\"", "Konnekt" , MB_ICONINFORMATION);
			ShellExecute(0 , "Explore" , profilesDir , "" , "" , SW_SHOW);				
		}
		gracefullExit();
	}
	// Zmienne œrodowiskowe
	SetEnvironmentVariable("KonnektPath" , appPath.substr(0 , appPath.size()-1).c_str());
	SetEnvironmentVariable("KonnektLog" , logPath.substr(0 , logPath.size()-1).c_str());
	SetEnvironmentVariable("KonnektData" , dataPath.substr(0 , dataPath.size()-1).c_str());

	//atexit(atexit_finish);
	InitInstance(hInstance);
	_mkdir(logPath);
#ifdef __DEBUG
	{
		WIN32_FIND_DATA fd;
		HANDLE hFile;
		if ((hFile = FindFirstFile(logPath + "*.log",&fd))!=INVALID_HANDLE_VALUE) {
			do {
				struct _stat st;
				CStdString file = logPath + fd.cFileName;
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || _stat(file , &st))
					continue;
				time_t curTime = time(0);
				if ((curTime - st.st_mtime) > 86400*2 || (st.st_size > 500000 && (curTime - st.st_mtime) > 3600))
					unlink(file);
			} while (FindNextFile(hFile , &fd));
			FindClose(hFile);
		}
		logFileName = string(logPath) + string("konnekt_live_") + string(cDate64(true).strftime("%y-%m-%d"));
	}
	CStdString logAdd = "";
	int logAddInt = 0;
	while (!_access(logFileName + logAdd + ".log" , 0)) {
		logAddInt++;
		_snprintf(logAdd.GetBuffer(5) , 5 , "[%02d]" , logAddInt);
		logAdd.ReleaseBuffer();
	}
	logFileName += logAdd + ".log";
	Debug::logFile = fopen(logFileName , "wt");
	if (Debug::logFile == 0) debug = false;

#if defined(__WITHDEBUGALL)
	Debug::debugAll = getArgV(ARGV_DEBUGALL);
#else
	Debug::debugAll = false;
#endif
#if defined(__DEBUGALL)
	Debug::debugAll = true;
#endif


	if (superUser) {
		Debug::startup(hInstance);
		Debug::ShowDebugWnds();
	} else {
		//debugLock=false;
		InitCommonControls();
	}
	if (Debug::logFile) {
		fprintf(Debug::logFile , "Konnekt_log ... v %s\n" , suiteVersionInfo.c_str());
		fprintf(Debug::logFile , "\n");
		if (Debug::debugAll) 
			fprintf(Debug::logFile , "Logowanie WSZYSTKICH zdarzeñ w³¹czone!\n");
		fprintf(Debug::logFile , "\n_____________________________________________\n\n");
		fflush(Debug::logFile);
	}
#else
#endif
	IMLOG("argv[0]=%s" , _argv[0]);
	IMLOG("appPath=%s" , appPath.c_str());
	IMLOG("dataPath=%s", dataPath.c_str());
	GetEnvironmentVariable("PATH", envPath.GetBuffer(255), 255);
	envPath.ReleaseBuffer();
	IMLOG("PATH=%s", envPath.c_str());
#if defined(__NOCATCH) && !defined(__PLUGCATCH)
	noCatch = true;
	WinMainBlock();
#else
	noCatch = getArgV(ARGV_NOCATCH);
	if (noCatch) {
		WinMainBlock();
	} else {
		WinMainSafeBlock2();
	}
#endif
	return 0;
}



};


WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return Konnekt::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
