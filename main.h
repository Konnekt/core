#pragma once

#ifndef __BETA
#define __BETA
#endif
#define __DEBUG
//#define __DEBUGALL
//#define __LOGFULL
//#define __NOCATCH
//#undef __NOCATCH
#define __PLUGCATCH
//#define __DEBUGMEM
#define MAX_LOADSTRING 2000

#ifdef __DEBUG
#define __TEST
#endif

#ifdef __NOCATCH
#define __DEV
#endif

#define XOR_KEY "\x16\x48\xf0\x85\xa9\x12\x03\x98\xbe\xcf\x42\x08\x76\xa5\x22\x84"  // NIE ZMIENIAÆ!!!!!!!
#define UI_CERT "\x10\x20 a\x60\xf0\x15\xae_uiw98"

#define MAX_PLUGS  100
#define TIMEOUT_DIAL  20000
#define TIMEOUT_RETRY Tables::cfg->getInt(0,CFG_TIMEOUT_RETRY)  // 60000


#define MYWM_CORE WM_USER + 28510
#define MYWM_CORECOUNT 20
//#define MYWM_IM             MYWM_CORE + 0
//#define MYWM_IMCORE         MYWM_CORE + 1
//#define MYWM_IMDIRECT       MYWM_CORE + 2
#define MYWM_MESSAGEQUEUE   MYWM_CORE + 10
#define MYWM_SETCONNECT   MYWM_CORE + 11

#define STACKTRACE() stackTrace = dcallstack()

using namespace Stamina;


namespace Konnekt {
	//DWORD curThread;
	//	CStdString versionSig;
	extern String appPath;
	extern String appFile;
	extern String sessionName; // ID sesji - MD5 katalogu z profilem
	extern String tempPath;
	extern String dataPath;
	extern HINTERNET hInternet; // uchwyt do globalnej sesji sieciowej...
	extern bool noCatch;

	//	bool suspended = false;

	extern String versionInfo;
	extern unsigned int versionNumber;
	extern String suiteVersionInfo;
	extern bool newVersion;

	//CStdString password;

	//const string core_v="W98";
	const string sig="Konnekt";


	extern HANDLE hMainThread;
	extern unsigned long MainThreadID; 
	extern unsigned int ui_sender;
	extern bool canQuit;
	extern bool running;
	extern bool isRunning;
	extern bool isWaiting;
	extern bool canWait; // Czy mo¿na czekaæ na cokolwiek (overlapped IO itd...)
	extern HANDLE semaphore;


	void initialize() ;
	void deinitialize(bool doExit = true) ;
	void end();
	void mainWindowsLoop(bool loop);
	inline void mainWindowsLoop() {
		mainWindowsLoop(false);
	}

	void loadRegistry();
	int saveRegistry();
	void __cdecl restart ( bool withProfile = true );
	void __cdecl removeProfileAndRestart ( void );

	void showArgumentsHelp(bool force);
	void initializePaths();
	void initializeSuiteVersionInfo();

	bool restoreRunningInstance();

	// ------------------------------ core_setup
	void setPlgColumns();
	void setColumns();
	void initializeMainTables();

	void updateCore(int from);

	void gracefullExit();
  void exit(int code);

};
using namespace Konnekt;




