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
#define TIMEOUT_RETRY Cfg.getint(0,CFG_TIMEOUT_RETRY)  // 60000

#define PLUG_MAX_COUNT 0x80

#define MYWM_CORE WM_USER + 28510
#define MYWM_CORECOUNT 20
//#define MYWM_IM             MYWM_CORE + 0
//#define MYWM_IMCORE         MYWM_CORE + 1
//#define MYWM_IMDIRECT       MYWM_CORE + 2
#define MYWM_MESSAGEQUEUE   MYWM_CORE + 10
#define MYWM_SETCONNECT   MYWM_CORE + 11

#define STACKTRACE() stackTrace = dcallstack()

#define PLG_FILE 0  //pchar
#define PLG_MD5  1  //pchar
#define PLG_SIG  2  //pchar
#define PLG_LOAD 3  //int
#define PLG_NEW  4  //int ns

#define C_PLG_COLCOUNT 5


namespace Konnekt {
	//DWORD curThread;
	//	CStdString versionSig;
	extern CStdString appPath;
	extern CStdString exePath;
	extern CStdString app;
	extern CStdString sessionName; // ID sesji - MD5 katalogu z profilem
	extern CStdString tempPath;
	extern CStdString dataPath;
	extern bool isComCtl6;
	extern HINTERNET hInternet; // uchwyt do globalnej sesji sieciowej...
	extern bool noCatch;

	//	bool suspended = false;

	extern CStdString versionInfo;
	extern unsigned int versionNumber;
	extern CStdString  suiteVersionInfo;
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


	void Init() ;
	void deInit(bool doExit = true) ;
	void end();
	void WMessages(bool loop = 0);
	inline void WMProcess2() {WMessages(false);}

	int saveRegistry();
	void __cdecl restart ( void );
	void __cdecl remove ( void );

	// ------------------------------ core_setup
	void setPlgColumns();
	void setColumns();
	void updateCore(int from);

	void gracefullExit();

};
using namespace Konnekt;




