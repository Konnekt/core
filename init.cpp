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
#include "mru.h"

#include <Stamina\Logger.h>
#include <Stamina\Exception.h>
#include "KLogger.h"

using namespace Stamina;

namespace Konnekt {

	const char* const coreLibraries = "/^(msvc.+|smemory|stamina|devil.*|zlib)\\.dll$/i";

	/*HACK: Przenoszenie bibliotek z katalogu g��wnego...*/
	void moveAdditionalLibraries() {
		FindFile ff(dataPath + "dll\\*.dll");
		ff.setFileOnly();
		while (ff.find()) {
			struct _stat stData, stMain;
			CStdString file = dataPath + "dll\\" + ff.found().getFileName();
			CStdString fileMain = ff.found().getFileName();
			// je�eli w kat. Konnekta nie ma tego dll'a, olewamy spraw�...
			// skoro ju� jeste�my w tym miejscu, znaczy �e s� wszystkie potrzebne
			// do rozruchu.
			if (fileExists(ff.found().getFileName().c_str()) == false) continue;
			// niekt�re DLLki po prostu musz� by� w g��wnym...
			if (RegEx::doMatch(coreLibraries, ff.found().getFileName().c_str()) > 0 ) continue;
			if (ff.found().getFileName() == "ui.dll") continue;

			// ko�czymy gdy jest katalogiem, lub nie mozemy odczytac czasow
			if (_stat(file , &stData) || _stat(fileMain , &stMain))
				continue;
			if (stMain.st_mtime <= stData.st_mtime) {
				unlink(fileMain + ".old");
				rename(fileMain, fileMain + ".old");
			} else {
				unlink(file + ".old");
				rename(file, file + ".old");
			}
		}
	}


//---------------------------------------------------------------------------

	void showArgumentsHelp(bool force) {
		if (force || getArgV(ARGV_HELP)) {
			string nfo;
			nfo += "Konnekt.exe mo�na uruchomi� z nast. parametrami:\r\n";
			nfo += "\r\n  " ARGV_RESTORE " tylko przekazuje parametry do otwartego okna Konnekta";
			nfo += "\r\n  " ARGV_PROFILE "=<nazwa profilu lub ?> : uruchamia program z wybranym profilem (? - okno wyboru).";
			nfo += "\r\n  " ARGV_PROFILESDIR " : Otwiera aktualny katalog profili.";
			nfo += "\r\n  " ARGV_PROFILESDIR "=<nowy katalog> : ustawia nowy katalog z profilami. Pliki profili nale�y przenie�� r�cznie w razie potrzeby.";
			nfo += "\r\n  " ARGV_LOGPATH "=<�cie�ka do katalogu> : zapisuje logi do wybranego katalogu.";
			nfo += "\r\n  " ARGV_PLUGINS " : wy�wietla okno ustawiania wtyczek.";
			nfo += "\r\n  " ARGV_OFFLINE " : wy��cza automatyczne ��czenie z sieci�";
	#ifdef __DEBUG
			nfo+= "\r\n  " ARGV_DEBUG " : uaktywnia menu do debugowania";
	#endif
			MessageBox(0 , nfo.c_str() , "Konnekt.exe - parametry" , MB_ICONINFORMATION);
			gracefullExit();
		}
	}


	void initializePaths() {
		GetModuleFileName(0 , appFile.GetBuffer(MAX_PATH) , MAX_PATH);
		appFile.ReleaseBuffer();

		// katalog aplikacji
		appPath = Stamina::getFileDirectory(appFile);
		if (!appPath.empty()) {
			_chdir(appPath.c_str());
		}
		appPath = Stamina::unifyPath(getCurrentDirectory(), true);

		appFile = appPath + getFileName(appFile);
		__argv[0] = (char*)appFile.c_str();


		// �cie�ka do danych
		dataPath = appPath + "data\\";

		// powi�kszenie PATH
		CStdString envPath;
		GetEnvironmentVariable("PATH", envPath.GetBuffer(1024), 1024);
		envPath.ReleaseBuffer();
		SetEnvironmentVariable("PATH", dataPath + "dll;" + envPath);
		_SetDllDirectory(dataPath + "dll");

		if (fileExists(dataPath) == false) {
			MessageBox(0, "Brakuje katalogu z danymi!\r\n" + dataPath, "Konnekt", MB_ICONERROR);
			gracefullExit();
		}

		/*HACK: Przenoszenie plik�w bety*/
		if ( fileExists("beta.dtb") == true && fileExists(dataPath + "beta\\beta.dtb") == false )  {
			createDirectories(dataPath + "beta");
			rename("beta.dtb", dataPath + "beta\\beta.dtb");
			rename("beta_reports.dtb", dataPath + "beta\\beta_reports.dtb");
			rename("beta_stats.dtb", dataPath + "beta\\beta_stats.dtb");
		}

		moveAdditionalLibraries();

		Debug::logPath = unifyPath(getArgV(ARGV_LOGPATH , true , dataPath + "log\\"), true);

		// zmienne �rodowiskowe...
		SetEnvironmentVariable("KonnektPath" , unifyPath(appPath, false).c_str());
		SetEnvironmentVariable("KonnektLog" , unifyPath(Debug::logPath, false).c_str());
		SetEnvironmentVariable("KonnektData" , unifyPath(dataPath, false).c_str());

	}




	void initializeSuiteVersionInfo() {
		FileVersion fv(appFile);
		versionInfo = fv.getFileVersion().getString();
		versionNumber = fv.getFileVersion().getInt();
		FileVersion fvUi("ui.dll");
		suiteVersionInfo = versionInfo + " | ui " + fvUi.getFileVersion().getString();
	}


//---------------------------------------------------------------------------	

	void initialize() {
		// dbg.showLog = true;

		Stamina::mainLogger = new Konnekt::KLogger(Stamina::logAll);


		srand( (unsigned)time( NULL ) );

		Unique::initializeUnique();

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
			restoreRunningInstance();
			gracefullExit();
		}


		WSADATA wsaData;
		WSAStartup( MAKEWORD( 2, 0 ), &wsaData );


		pluginsInit();


		canQuit = false;
		// Ladowanie DLL'i

		// Core
		try {
			plugins.plugInClassic(appPath, coreIMessageProc, pluginUI);
		} catch (Exception& e) {
			CStdString msg;
			msg.Format("Ten b��d si� nie zdarza cz�sto, ale sam siebie nie mog� za�adowa�!!!\r\n\r\n%s", e.getReason().a_str());
			MessageBox(NULL, msg.c_str(), loadString(IDS_APPNAME).c_str(),MB_ICONERROR|MB_OK|MB_TASKMODAL ); 
			exit(0); 
		}

		// UI
		try {
			plugins.plugInClassic("ui.dll", 0, pluginUI);
		} catch (Exception& e) {
			CStdString msg;
			msg.Format(loadString(IDS_ERR_NOUI).c_str(), e.getReason().a_str());
			MessageBox(NULL, msg.c_str(), loadString(IDS_APPNAME).c_str(),MB_ICONERROR|MB_OK|MB_TASKMODAL ); 
			exit(0); 
		}

		IMLOG("--- UI loaded ---");
		if (profilesDir.empty()) setProfilesDir();
		IMLOG("--- ProfilesDir set ---");
		if (!setProfile(true , true)) 	gracefullExit();
		IMLOG("--- Profile set ---");
		IMLOG("profile=%s", profile.c_str());
		saveRegistry();

		// Sprawdzamy teraz, czy przypadkiem jakis inny K nie bawi sie juz tym profilem...
		GetFullPathName(profileDir.c_str() , 1024 , sessionName.GetBuffer(1024) , 0);
		IMLOG("profileDir=%s", profileDir.c_str());

		// ustawiamy �cie�k� plik�w tymczasowych
		tempPath = (string(getenv("TEMP")) + "\\Konnekt_"+string(profile)+"_"+string(MD5Hex(CStdString(appPath + profileDir).ToLower())).substr(0, 8));
		Stamina::createDirectories(tempPath);
		SetEnvironmentVariable("KonnektTemp" , tempPath);
		IMLOG("tempDir=%s", tempPath.c_str());


		sessionName.ReleaseBuffer();
		sessionName.ToLower();
		sessionName = MD5Hex((char*)sessionName.c_str()).substr(0 , 8);


		semaphore = CreateSemaphore(0 , 1 , 1 , "KONNEKT." + sessionName);
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			if (!restoreRunningInstance())
				MessageBox(0 , loadString(IDS_ERR_ONEINSTANCE).c_str() , "Konnekt" , MB_OK | MB_ICONERROR);
			exit(0);
		}
		SetEnvironmentVariable("KonnektProfile" , profileDir.substr(0 , profileDir.size()-1).c_str());
		SetEnvironmentVariable("KonnektUser" , profile.c_str());

		initializeMainTables();

		MRU::init();

		// Ustawia kolumny dla Core
		setColumns();
		// Ustawia kolumny Core/UI
		IMessage(IM_SETCOLS, Net::Broadcast().onlyNet(Net::core), imtAll);
		IMLOG("--- Core columns set ---");

		// �aduje pluginy...
		setPlugins();
		IMessage(IM_ALLPLUGINSINITIALIZED , NET_BROADCAST , IMT_ALL);
		IMLOG("--- Plugins loaded ---");

		// zamawianie kolumn - spos�b nowoczesny
		Tables::cfg->requestColumns(Ctrl);
		Tables::cnt->requestColumns(Ctrl);

		// Ustawia inne wtyczki
		IMessage(IM_SETCOLS, Net::Broadcast().notNet(Net::core), imtAll);


		IMLOG("--- Plugin columns set ---");
		loadProfile();
		Contacts::updateAllContacts();
		IMLOG("--- Profile loaded ---");

		// Wy�wietla okno ustalania "trudno�ci" (o ile zajdzie taka potrzeba...)
		ICMessage(IMI_SET_SHOWBITS , 0 , 0);

		hInternet = (HINTERNET) ICMessage(IMC_HINTERNET_OPEN , (int)"Konnekt" , 0);

		checkVersions();
		IMLOG("--- Versions checked ---");
		Tables::cfg->setString(0, CFG_APPFILE, appFile);
		Tables::cfg->setString(0, CFG_APPDIR, appPath);
		IMessage(IM_UI_PREPARE, Net::broadcast, imtAll);
		IMLOG("--- UI prepared ---");
		IMessage(IM_START, Net::Broadcast().notNet(Net::core), imtAll);
		IMessage(IM_START, Net::Broadcast().onlyNet(Net::core));
		IMLOG("--- Plugins started ---");
		if (newProfile) {
			IMessage(IM_NEW_PROFILE , Net::broadcast , imtAll);
			IMLOG("--- NEW profile passed ---");
		}

		Connections::startUp();
		IMessage(IM_NEEDCONNECTION,Net::broadcast,imtProtocol);
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
		// if (Tables::cfg->getInt(0 , CFG_AUTO_CONNECT))
		//   IMessage(IM_CONNECT , NET_BC , IMT_PROTOCOL);

		// Rozeslanie plug_args
		IMessage(&sIMessage_plugArgs(__argc , __argv));

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
			IMessage(IM_DISCONNECT, Net::broadcast, imtProtocol);
			IMessage(IM_END , Net::Broadcast().setReverse(),imtAll, !canWait);
		}
	}

	void deinitialize(bool doExit) {
	#ifdef __DEBUG
		Debug::showLog = false;
	#endif
		static DWORD startTime=0;
		if (startTime != 0) {
			IMLOG("- deinitialize in progress for %d ms" , GetTickCount() - startTime);
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

		// odpinamy wszystkie wtyczki...
		for (int i = plugins.count() - 1; i >= 0; --i) {
			plugins.plugOut(plugins[i], false);
		}
		IMLOG("-plugs unpluged");

		Tables::deinitialize();


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

		// czy�cimy list� plugin�w
		plugins.cleanUp();


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


};
