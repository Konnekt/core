#pragma once
#include "main.h"
#include "konnekt_sdk.h"

namespace Konnekt{ namespace Debug {

	struct sIMDebug {
		string id, p1, p2, result, error
			, sender, receiver,
			nr , net , type , thread
			;


	};


	extern bool superUser;
	extern CStdString logPath;
	extern CStdString logFileName; // nazwa pliku z logiem
	extern FILE * logFile;
	extern bool IMfinished;
	extern string stackTrace;
	extern unsigned int threadId;

	extern int x;
	extern int y;
	extern int w;
	extern int h;
	extern bool show;
	extern bool log;
	extern bool scroll;
	extern bool logAll;
	extern bool debugAll;

#ifdef __DEBUG
	int IMDebug(sIMessage_base * msgBase , unsigned int , int);
	int IMDebugResult(sIMessage_base * msgBase , int pos , int res , int=0);
	int IMDebug_transform(sIMDebug & IMD , sIMessage_base * msgBase , int result , int error);
#endif


	// -------------------------------  debug_window


	extern bool showLog;
	extern bool debug;
	extern HWND hwnd;
	extern cCriticalSection windowCSection;

	void startup(HINSTANCE hInst);
	void finish();

	void debugLog();
	void debugLogInfo();
	void debugLogQueue();
	void debugLogMsg(string msg);
	void debugLogValue(string name , string value);
	void ShowDebugWnds();
	void debugLogStart(sIMDebug & IMD , sIMessage_base * msg , string ind);
	void debugLogEnd(sIMDebug & IMD , sIMessage_base * msg , bool multiline , string ind);
};};