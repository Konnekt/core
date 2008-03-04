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
#include "tables.h"
#include "message_queue.h"
#include <Stamina\Logger.h>
#include <Stamina\Exception.h>
#include <Stamina\iArray.h>
#include "KLogger.h"

using namespace Stamina;
using namespace Messages;

extern "C" __declspec(dllexport) void __stdcall KonnektApiRegister(fApiVersionCompare reg) {
	using namespace Stamina;
	reg(iObject::staticClassInfo().getModuleVersion());
	reg(iSharedObject::staticClassInfo().getModuleVersion());
	reg(iLockableObject::staticClassInfo().getModuleVersion());
	reg(Stamina::Lib::version);
	reg(Konnekt::apiVersion);
	reg(iString::staticClassInfo().getModuleVersion());
	reg(StringRef::staticClassInfo().getModuleVersion());
	reg(String::staticClassInfo().getModuleVersion());
	reg(Lock::staticClassInfo().getModuleVersion());
	reg(iArrayBase::staticClassInfo().getModuleVersion());
	reg(Exception::staticClassInfo().getModuleVersion());

	reg(Konnekt::iPlugin::staticClassInfo().getModuleVersion());
	reg(Konnekt::Tables::iTable::staticClassInfo().getModuleVersion());
	reg(Stamina::DT::iColumn::staticClassInfo().getModuleVersion());
	reg(Stamina::DT::iRow::staticClassInfo().getModuleVersion());
	reg(Stamina::Unique::iRange::staticClassInfo().getModuleVersion());
	reg(Stamina::Unique::iDomain::staticClassInfo().getModuleVersion());
	reg(Stamina::Unique::iDomainList::staticClassInfo().getModuleVersion());
}



namespace Konnekt {

	const char* const coreLibraries = "/^(msvc.+|smemory|stamina|devil.*|zlib)\\.dll$/i";

	/*HACK: Przenoszenie bibliotek z katalogu g³ównego...*/
	void moveAdditionalLibraries() {
		FindFile ff(dataPath + "dll\\*.dll");
		ff.setFileOnly();
		while (ff.find()) {
			struct _stat stData, stMain;
			String file = dataPath + "dll\\" + ff.found().getFileName();
			String fileMain = ff.found().getFileName();
			// je¿eli w kat. Konnekta nie ma tego dll'a, olewamy sprawê...
			// skoro ju¿ jesteœmy w tym miejscu, znaczy ¿e s¹ wszystkie potrzebne
			// do rozruchu.
			if (fileExists(ff.found().getFileName().c_str()) == false) continue;
			// niektóre DLLki po prostu musz¹ byæ w g³ównym...
			if (RegEx::doMatch(coreLibraries, ff.found().getFileName().c_str()) > 0 ) continue;
			if (ff.found().getFileName() == "ui.dll") continue;

			// koñczymy gdy jest katalogiem, lub nie mozemy odczytac czasow
			if (_stat(file.a_str() , &stData) || _stat(fileMain.a_str() , &stMain))
				continue;
			if (stMain.st_mtime <= stData.st_mtime) {
				_unlink((fileMain + ".old").a_str());
				rename(fileMain.a_str(), (fileMain + ".old").a_str());
			} else {
				_unlink((file + ".old").a_str());
				rename(file.a_str(), (file + ".old").a_str());
			}
		}
	}


//---------------------------------------------------------------------------

	void showArgumentsHelp(bool force) {
		if (force || argVExists(ARGV_HELP)) {
			String nfo;
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
	}


	void initializePaths() {
		GetModuleFileName(0 , appFile.useBuffer<char>(MAX_PATH) , MAX_PATH);
		appFile.releaseBuffer<char>();

		// katalog aplikacji
		appPath = Stamina::getFileDirectory(appFile);
		if (!appPath.empty()) {
			_chdir(appPath.c_str());
		}
		appPath = Stamina::unifyPath(getCurrentDirectory(), true);

		appFile = appPath + getFileName(appFile);
		__argv[0] = (char*)appFile.c_str();


		// œcie¿ka do danych
		dataPath = appPath + "data\\";

		// powiêkszenie PATH
		CStdString envPath;
		GetEnvironmentVariable("PATH", envPath.GetBuffer(1024), 1024);
		envPath.ReleaseBuffer();
		SetEnvironmentVariable("PATH", (dataPath + "dll;" + envPath.c_str()).c_str());
		_SetDllDirectory((dataPath + "dll").c_str());

		if (fileExists(dataPath) == false) {
			MessageBox(0, ("Brakuje katalogu z danymi!\r\n" + dataPath).c_str(), "Konnekt", MB_ICONERROR);
			gracefullExit();
		}

		/*HACK: Przenoszenie plików bety*/
		if ( fileExists("beta.dtb") == true && fileExists(dataPath + "beta\\beta.dtb") == false )  {
			createDirectories(dataPath + "beta");
			rename("beta.dtb", (dataPath + "beta\\beta.dtb").c_str());
			rename("beta_reports.dtb", (dataPath + "beta\\beta_reports.dtb").c_str());
			rename("beta_stats.dtb", (dataPath + "beta\\beta_stats.dtb").c_str());
		}

		moveAdditionalLibraries();

		Debug::logPath = unifyPath(getArgV(ARGV_LOGPATH , true , dataPath + "log\\"), true);

		// zmienne œrodowiskowe...
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

		srand( (unsigned)time( NULL ) );

		Unique::initializeUnique();

		MainThreadID = GetCurrentThreadId();
		hMainThread = 0;
		DuplicateHandle(GetCurrentProcess() , GetCurrentThread()
			, GetCurrentProcess() , &hMainThread
			, 0 , 0 , DUPLICATE_SAME_ACCESS);

		// FileVersionInfo(_argv[0] , versionInfo);
		setlocale( LC_ALL, "Polish" );
		// dla restore
		if (argVExists(ARGV_RESTORE)) {
			restoreRunningInstance();
			gracefullExit();
		}


		WSADATA wsaData;
		WSAStartup( MAKEWORD( 2, 0 ), &wsaData );

		canQuit = false;
		// Ladowanie DLL'i

		// Core
		try {
			plugins.plugInClassic(appFile, coreIMessageProc, pluginCore);
		} catch (Exception& e) {
			CStdString msg;
			msg.Format("Ten b³¹d siê nie zdarza czêsto, ale sam siebie nie mogê za³adowaæ!!!\r\n\r\n%s", e.getReason().a_str());
			MessageBox(NULL, msg.c_str(), loadString(IDS_APPNAME).c_str(),MB_ICONERROR|MB_OK|MB_TASKMODAL ); 
			exit(0); 
		}

		Stamina::mainLogger = plugins[pluginCore].getLogger();

	#ifdef __BETA
		Beta::init();
	#endif

		if (argVExists("-makeReport")) {
			String value = getArgV("-makeReport", true, "");
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
			Beta::makeReport(value.c_str(), true, useDate, silent);
			//Tables::deinitialize();
			gracefullExit();
		}

		// UI
		try {
			plugins.plugInClassic("ui.dll", 0, pluginUI);
		} catch (Exception& e) {
			CStdString msg;
			msg.Format(loadString(IDS_ERR_NOUI).c_str(), e.getReason().a_str());
			MessageBox(NULL, msg.c_str(), loadString(IDS_APPNAME).c_str(),MB_ICONERROR|MB_OK|MB_TASKMODAL ); 
			//Tables::deinitialize();
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
		GetFullPathName(profileDir.c_str() , 1024 , sessionName.useBuffer<char>(1024) , 0);
		IMLOG("profileDir=%s", profileDir.c_str());

		// ustawiamy œcie¿kê plików tymczasowych
		tempPath = getEnvironmentVariable("TEMP") + "\\Konnekt_" + profile + "_" + MD5Hex((appPath + profileDir).toLower()).substr(0, 8);
		Stamina::createDirectories(tempPath);
		setEnvironmentVariable("KonnektTemp" , tempPath);
		IMLOG("tempDir=%s", tempPath.c_str());


		sessionName.releaseBuffer<char>();
		sessionName.makeLower();
		sessionName = MD5Hex(sessionName).substr(0 , 8);


		semaphore = CreateSemaphore(0 , 1 , 1 , ("KONNEKT." + sessionName).c_str());
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			if (!restoreRunningInstance())
				MessageBox(0 , loadString(IDS_ERR_ONEINSTANCE).c_str() , "Konnekt" , MB_OK | MB_ICONERROR);

			//Tables::deinitialize();
			exit(0);
		}
		SetEnvironmentVariable("KonnektProfile" , profileDir.substr(0 , profileDir.size()-1).c_str());
		SetEnvironmentVariable("KonnektUser" , profile.c_str());

		initializePluginsTable();
		initializeMainTables();

		MRU::init();

		// Ustawia kolumny dla Core
		setColumns();
		// Ustawia kolumny Core/UI
		IMessage(IM_SETCOLS, Net::Broadcast().onlyNet(Net::core), imtAll);
		IMLOG("--- Core columns set ---");

		// £aduje pluginy...
		Plugins::setPlugins();
    IMessage(IM_ALLPLUGINSINITIALIZED, Net::broadcast, imtAll);
		IMLOG("--- Plugins loaded ---");

		// zamawianie kolumn - sposób nowoczesny
		Tables::cfg->requestColumns(Ctrl);
		Tables::cnt->requestColumns(Ctrl);

		// Ustawia inne wtyczki
		IMessage(IM_SETCOLS, Net::Broadcast().notNet(Net::core), imtAll);


		IMLOG("--- Plugin columns set ---");
		loadProfile();
		Contacts::updateAllContacts();
		IMLOG("--- Profile loaded ---");

		// Wyœwietla okno ustalania "trudnoœci" (o ile zajdzie taka potrzeba...)
		ICMessage(IMI_SET_SHOWBITS , 0 , 0);

		hInternet = (HINTERNET) ICMessage(IMC_HINTERNET_OPEN , (int)"Konnekt" , 0);

		Plugins::checkVersions();
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

		// Podlaczenie starej kolejki wiadomosci
		for (Plugins::tList::reverse_iterator it = plugins.rbegin(); it != plugins.rend(); ++it) {
			Plugin& plugin = **it;
			if (plugin.getId() == pluginUI) break;

			if ((plugin.getType() & imtAllMessages) || (plugin.getType() & imtMessage)) {

				mhlist.registerHandler(new OldPluginMessageHandler(plugin.getNet(), 
					plugin.getId()), plugin.getPriority());
			}
		}

		Connections::startUp();
		IMessage(IM_NEEDCONNECTION,Net::broadcast,imtProtocol);
		Connections::connect();
		IMLOG("--- Auto-Connected ---");

		MessageQueue* mq = MessageQueue::getInstance();
		mq->init();
		mq->loadMessages();
		mq->runMessageQueue(&MessageSelect(Net::broadcast, 0, Message::typeAll, Message::flagNone, Message::flagSend));

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
		IMessage(IM_BEFOREEND , Net::broadcast, imtAll, !canWait , 0);
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
			if (plugins[i].isVirtual()) continue;
			plugins.plugOut(plugins[i], false);
		}

		IMLOG("-plugs unpluged");

		mhlist.clear();
		MessageQueue::getInstance()->deinit();

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
		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n______________________\n\nTerminate [%d ms]\n" , GetTickCount() - startTime);
		}
		Debug::finish();
	#endif
		//	return msg.wParam;

		// czyœcimy listê pluginów
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
