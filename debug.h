#pragma once
#include "main.h"
#include "konnekt_sdk.h"
#include "plugins.h"

namespace Konnekt{ namespace Debug {

	class IMessageInfo {
	public:
		IMessageInfo(sIMessage_base* msg) {
			_msg = msg;
		}
		StringRef getId() {
			return PassStringRef( getId(_msg->id) );
		}
		StringRef getNet() {
			return PassStringRef( getNet(_msg->net) );
		}
		StringRef getType() {
			return PassStringRef( getType(_msg->type) );
		}
		StringRef getSender() {
			return PassStringRef( getPlugin(_msg->sender) );
		}
		String getData();
		String getResult(int result);
		static StringRef getId(tIMid id);
		static StringRef getNet(tNet net);
		static StringRef getBroadcast(tNet net);
		static StringRef getType(enIMessageType type);
		static StringRef getThread(int threadId = -1);
		static StringRef getError(enIMessageError error);
		static StringRef getPlugin(tPluginId plugin);
		static StringRef getPlugin(Plugin& plugin);

	private:
		sIMessage_base* _msg;

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

	extern int imessageCount;

#ifdef __DEBUG
	int logIMessage(sIMessage_base* msg, Plugin& receiver);
	void logIMessageResult(sIMessage_base * msg, int pos, int result);

	void logIMessageBC(sIMessage_base* msg);
	void logIMessageBCResult(sIMessage_base * msg, int result, int hit);
#endif

	string logIndent(int offset = 0, char tab = '\t');

	// -------------------------------  debug_window


	extern bool showLog;
	extern bool debug;
	extern HWND hwnd;
	extern Stamina::CriticalSection windowCSection;

	void startup(HINSTANCE hInst);
	void finish();

	void initializeDebug();

	void debugLog();
	void debugLogInfo();
	void debugLogQueue();
	void debugLogMsg(string msg);
	void debugLogValue(string name , string value);
	void ShowDebugWnds();
	void debugLogMsg(Plugin& plugin, LogLevel level, const char* module, const char* where, const StringRef& msg);
	void debugLogIMStart(sIMessage_base * msg, Plugin& receiver);
	void debugLogIMEnd(sIMessage_base * msg, int result, bool multiline);
	void debugLogIMBCStart(sIMessage_base * msg);
	void debugLogIMBCEnd(sIMessage_base * msg, int result, int hit);
};};