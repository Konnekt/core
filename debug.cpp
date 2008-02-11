#include "stdafx.h"
#include "debug.h"
#include "beta.h"
#include "plugins.h"
#include "tables.h"
#include "imessage.h"
#include "threads.h"
#include "argv.h"
#include "klogger.h"

#define LOG_TAB 9

using namespace Stamina;

namespace Konnekt { namespace Debug {

	bool superUser=false;
	CStdString logPath;
	CStdString logFileName; // nazwa pliku z logiem
	FILE * logFile;
	int imessageCount = 0;
	string stackTrace="";
	unsigned int threadId = 0;
	bool debugAll = false;


	void initializeDebug() {
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
						_unlink(file);
				} while (FindNextFile(hFile , &fd));
				FindClose(hFile);
			}
			logFileName = string(logPath) + string("konnekt_live_") + Date64(true).strftime("%y-%m-%d");
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
		Debug::debugAll = argVExists(ARGV_DEBUGALL);
	#else
		Debug::debugAll = false;
	#endif
	#if defined(__DEBUGALL)
		Debug::debugAll = true;
	#endif
		if (superUser) {
			Debug::startup(Stamina::getHInstance());
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

		IMLOG("argv[0]=%s" , __argv[0]);
		IMLOG("appPath=%s" , appPath.c_str());
		IMLOG("dataPath=%s", dataPath.c_str());
		IMLOG("PATH=%s", getEnvironmentVariable("PATH").c_str());

	}


#ifdef __DEBUG

	map <int , int> indent;
	bool debugLock = false;
	CRITICAL_SECTION section_debug;

	struct initializer {
		initializer() {
			InitializeCriticalSection(&section_debug);
		};
		~initializer() {
			DeleteCriticalSection(&section_debug);
		}
	} __initializer;


// -------------------------------------------------------------------------

	String IMessageInfo::getData() {
		int p1;
		int p2;
		if (_msg->s_size >= sizeof(sIMessage_2params)) {
			p1 = static_cast<sIMessage_2params*>(_msg)->p1;
			p2 = static_cast<sIMessage_2params*>(_msg)->p2;
		} else {
			p1 = 0; p2 = 0;
		}
		switch (_msg->id) {
			case IMI_WARNING:
			case IMI_ERROR :
				return stringf("\"%.2000s\"", p1);
			case IMI_ACTION : 
			case IMI_ACTION_SET :
				{
				if (!p1) return "B³¹d!";
				sUIActionInfo* ai = (sUIActionInfo*)p1;
				return stringf("act(%d / %d / %x), txt=%s, mask=%x, p2=%x", ai->act.id, ai->act.parent, ai->act.cnt, (ai->mask & UIAIM_TXT) ? ai->txt : 0, ai->mask, p2);
				}
			case Message::IM::imcNewMessage:
			case Message::IM::imSendMessage:
			case Message::IM::imReceiveMessage:
			case Message::IM::imOpenMessage:
				{
				if (!p1) return "B³¹d!";
				Message * m = (Message *) p1;
				return stringf("id=%d net=%d from[\"%.50s\"] to[\"%.50s\"]" , m->getId() , m->getNet() , m->getFromUid().a_str() , m->getToUid().a_str());
				}
			case IMC_CNT_IGNORED :
			case IMC_FINDCONTACT :
			case IMC_IGN_DEL :
				return stringf("%d @ \"%.2000s\"" , p1, p2);
			case IM_UIACTION :
				{
				if (!p1) return "B³¹d!";
				sUIActionNotify_base* an = (sUIActionNotify_base*)p1;
				return stringf("%d / %d / %x = %d" , an->act.parent,an->act.id , an->act.cnt , an->code);
				}
		}
		if (_msg->s_size >= sizeof(sIMessage_2params)) {
			return stringf("0x%x, 0x%x", p1, p2);
		} else {
			return "";
		}
	}
	String IMessageInfo::getResult(int result) {
		String s = "0x" + inttostr(result, 16);
		return PassStringRef( s );
	}
	StringRef IMessageInfo::getId(tIMid id) {
		switch (id) {
			case IM_PLUG_INIT:	return "IM_PLUG_INIT";
			case IMI_WARNING :  return "IMI_WARNING";
			case IMI_ERROR :	return "IMI_ERROR";
			case IMI_PREPARE :	return "IMI_PREPARE";
			case IMI_ACTION :	return "IMI_ACTION";
			case IMI_ACTION_SET:return "IMI_ACTION_SET";
			case IMI_REFRESH_LST:return "IMI_REFRESH_LST";
			case IMI_REFRESH_CNT:return "IMI_REFRESH_CNT";
			case IMI_NOTIFY:	return "IMI_NOTIFY";
			case IMI_NEWNOTIFY:	return "IMI_NEWNOTIFY";
			case MessageNotify::IM::imcMessageNotify:return "IMC_MESSAGENOTIFY";
			case IMC_SHUTDOWN :	return "IMC_SHUTDOWN";
			case Message::IM::imcNewMessage :return "IMC_NEWMESSAGE";
			case MessageSelect::IM::imcMessageQueue:return "IMC_MESSAGEQUEUE";
			case MessageSelect::IM::imcMessageWaiting:return "IMC_MESSAGEWAITING";
			case MessageSelect::IM::imcMessageRemove:return "IMC_MESSAGEREMOVE";
			case IMC_CNT_IGNORED:return "IMC_CNT_IGNORED";
			case IMC_FINDCONTACT:return "IMC_FINDCONTACT";
			case IMC_PROFILEDIR:return "IMC_PROFILEDIR";
			case IMC_CNT_REMOVE:return "IMC_CNT_REMOVE";
			case IMC_CFG_SETCOL:return "IMC_CFG_SETCOL";
			case IMC_CNT_SETCOL:return "IMC_CNT_SETCOL";
			case IMI_MSG_OPEN :return "IMI_MSG_OPEN";
			case IM_START:		return "IM_START";
			case IM_END:		return "IM_END";
			case IM_UIACTION :	return "IM_UIACTION";
			case IM_CONNECT :	return "IM_CONNECT";
			case IM_UI_PREPARE:	return "IM_UI_PREPARE";
			case Message::IM::imOpenMessage:	return "IM_MSG_OPEN";
			case Message::IM::imSendMessage:	return "IM_MSG_SEND";
			case Message::IM::imReceiveMessage:	return "IM_MSG_RCV";
			case IM_CNT_STATUSCHANGE:return "IM_CNT_STATUSCHANGE";
			case IMC_IGN_DEL:	return "IMC_IGN_DEL";
			case IM_AWAY:		return "IM_AWAY";
			case IM_BACK :		return "IM_BACK";
		}
		String s;
		if (id < IMI_BASE) { // core
			s = inttostr(id, 10);
		} else if (id < IM_BASE) { // imi
			s = "IMI_BASE+" +  inttostr(id - IMI_BASE, 10);
		} else {
			s = "IM_BASE+" +  inttostr(id - IM_BASE, 10);
		}
		return PassStringRef( s );
	}

	StringRef IMessageInfo::getNet(tNet net) {
		String s;
		if (net < Net::last) {
			s = inttostr(net, 10);
		} else {
			Net::Broadcast bc (net);
			if (bc.isReverse()) s += "-";
			if (bc.isBroadcast()) s += "BC";
			if (bc.isFirstOnly()) s += "FS";
			if (bc.getOnlyNet() != Net::all) s += "+" + inttostr(bc.getOnlyNet(), 10);
			if (bc.getNotNet() != Net::all) s += "-" + inttostr(bc.getNotNet(), 10);
			if (bc.getIMType() == Net::Broadcast::imtypeAny) s += "&any";
			if (bc.getIMType() == Net::Broadcast::imtypeNot) s += "&not";
			if (bc.getStartPlugin() != 0) s += "@0x" + inttostr(bc.getStartPlugin(), 16);
			if (bc.getStartOccurence() != 0) s += "#" + inttostr(bc.getStartOccurence(), 10);
			if (bc.getResultType() == Net::Broadcast::resultAnd) s += "=and";
			if (bc.getResultType() == Net::Broadcast::resultOr) s += "=or";
			if (bc.getResultType() == Net::Broadcast::resultSum) s += "=sum";
		}
		return PassStringRef( s );
	}

	StringRef IMessageInfo::getBroadcast(tNet net) {
		String s;
		Net::Broadcast bc (net);
		if (bc.isBroadcast()) s += "BROADCAST";
		if (bc.isFirstOnly()) s += "FIRST";
		if (bc.getOnlyNet() != Net::all) s += " +net=" + inttostr(bc.getOnlyNet(), 10);
		if (bc.getNotNet() != Net::all) s += " -net=" + inttostr(bc.getNotNet(), 10);
		if (bc.getIMType() == Net::Broadcast::imtypeAny) s += " type:any";
		if (bc.getIMType() == Net::Broadcast::imtypeNot) s += " type:not";
		if (bc.getStartPlugin() != 0) s += " startPlugin=0x" + inttostr(bc.getStartPlugin(), 16);
		if (bc.getStartOccurence() != 0) s += " start=" + inttostr(bc.getStartOccurence(), 10);
		if (bc.getResultType() == Net::Broadcast::resultAnd) s += " result:and";
		if (bc.getResultType() == Net::Broadcast::resultOr) s += " result:or";
		if (bc.getResultType() == Net::Broadcast::resultSum) s += " result:sum";
		if (bc.isReverse()) s += " reverse";
		return PassStringRef( s );
	}


	StringRef IMessageInfo::getType(enIMessageType type) {
		switch (type) {
			case imtAll: return "all";
			case imtNone: return "none";
		}
		String s = "0x" + inttostr(type, 16);
		return PassStringRef( s );
	}
	StringRef IMessageInfo::getThread(int threadId) {
		StringRef name;
		if (threadId == -1) {
			threadId = GetCurrentThreadId();
			name = TLSU().getName();
		} else {
			LockerCS lock (threadsCS);
			name = threads[threadId].name;
		}
		if (!name.empty()) {
			return PassStringRef( name );
		}
		String s = inttostr(threadId, 16);
		return PassStringRef( s );
	}
	StringRef IMessageInfo::getError(enIMessageError error) {
		switch (error) {
			case Konnekt::errorNone: return "";
			case Konnekt::errorNoResult: return "NoResult";
			case Konnekt::errorThreadSafe: return "TS";
		}
		String s = "0x" + inttostr(error, 16, 2, true);
		return PassStringRef( s );
	}
	StringRef IMessageInfo::getPlugin(tPluginId plugin) {
		return PassStringRef( plugins.getSig(plugin) );
	}
	StringRef IMessageInfo::getPlugin(Plugin& plugin) {
		return plugin.getSig().getRef();
	}


// -------------------------------------------------------------------------

	CriticalSection debugCS;

	void logTime() {
		static Stamina::Time64 lastLog(false);
		Stamina::Time64 now(true);
		// co 5 minut, lub o ka¿dej pe³nej godzinie wstawia pe³n¹ datê
		if (((__int64)(now - lastLog) > 60*5) || ((int)(lastLog / 86400) < (int)(now / 86400))) {
			fprintf(Debug::logFile, "[%s] ------------ \n", now.strftime("%c").c_str());
		}
		lastLog = now;
	}

	string logIndent(int offset, char tab) {
		string indent;
		if (Debug::logAll) {
			indent.resize( max(0, TLSU().stack.count() + offset), tab );
		}
		return indent;
	}

	int logIMessage(sIMessage_base * msg, Plugin& receiver) {
		if (!debug) return -1;
		if (!Debug::debugAll) return -1;

		Locker lock(debugCS);

		IMessageInfo info(msg);

		if (Debug::logFile) {

			if (!IMfinished) fprintf(Debug::logFile , "\n");

			logTime();

			string thread;
			if (mainThread.isCurrent() == false) {
				thread = "/" + IMessageInfo::getThread();
			}

			fprintf(Debug::logFile , "[%s:%03d]%s [%d%s] %s -> %s\t | %s\t | [%s]\t | %s"
				, Time64(true).strftime("%M:%S").c_str()
				, GetTickCount() % 1000
				, logIndent().c_str() 
				, imessageCount
				, thread.c_str()
				, info.getPlugin(msg->sender).c_str() 
				, info.getPlugin(receiver).c_str()
				, info.getNet().c_str()
				, info.getId().c_str()
				, info.getData().c_str()
				);
			fflush(Debug::logFile);
		}

		debugLogIMStart(msg, receiver);

		IMfinished = false;

		imessageCount ++;

		return imessageCount-1;
	}

	void logIMessageResult(sIMessage_base * msg, int pos, int result) {
		if (!debug || Debug::logFile == 0) return;
		if (!Debug::debugAll) return;
		if (pos < 0) return;

		LockerCS lock (debugCS);

		IMessageInfo info(msg);

		if (pos == imessageCount-1) {
			debugLogIMEnd(msg, result, false);
			fprintf(Debug::logFile , "\t= %s %s\n"
				, info.getResult(result).c_str()
				, info.getError(TLSU().stack.getError()).c_str()
				);
			IMfinished = true;
		} else {
			IMfinished = true;
			debugLogIMEnd(msg, result, true);
			if (Debug::logFile) {
				fprintf(Debug::logFile , "[--:--:---]%s = %s %s (%s)\n" , logIndent().c_str(), info.getResult(result).c_str(), info.getError(TLSU().stack.getError()).c_str(), info.getThread().c_str());
			}
		}
		fflush(Debug::logFile);
	}



	void logIMessageBC(sIMessage_base * msg) {
		if (!debug) return;
		if (!Debug::debugAll) return;
		Locker lock(debugCS);
		IMessageInfo info(msg);
		if (Debug::logFile) {
			if (!IMfinished) fprintf(Debug::logFile , "\n");
			logTime();
			fprintf(Debug::logFile , "[%s:%03d]%s %s (%s, %s) [%s]\t | %s :\n"
				, Time64(true).strftime("%M:%S").c_str()
				, GetTickCount() % 1000
				, logIndent().c_str()
				, info.getPlugin(msg->sender).c_str()
				, info.getBroadcast(msg->net).c_str()
				, info.getType().c_str()
				, info.getId().c_str()
				, info.getData().c_str()
				);
			fflush(Debug::logFile);
		}
		debugLogIMBCStart(msg);
	}


	void logIMessageBCResult(sIMessage_base * msg, int result, int hit) {
		if (!debug || Debug::logFile == 0) return;
		if (!Debug::debugAll) return;
		LockerCS lock (debugCS);
		IMessageInfo info(msg);
		IMfinished = true;
		debugLogIMBCEnd(msg, result, hit);
		if (Debug::logFile) {
			fprintf(Debug::logFile , "[--:--:---]%s === %s from %d (%s)\n" , logIndent().c_str(), 
				info.getResult(result).c_str(), hit, info.getThread().c_str());
		}
		fflush(Debug::logFile);
	}



#endif


}; // namespace Debug


	void KLogger::logMsg(Stamina::LogLevel level, const char* module, const char* where, const StringRef& msg) {
		using namespace Debug;
		if (Debug::logFile) {
			LockerCS lock(debugCS);
			logTime();

			string thread;
			if (mainThread.isCurrent() == false) {
				thread = "/" + IMessageInfo::getThread();
			}

			AString txt;
			txt.assignCheapReference(msg);

			//             if (ch[strlen(ch)-1]=='\n') ch[strlen(ch)-1]=0;
			if (txt.length() > 1 && txt[0] != txt[2]) {
				char * prefix = 0;
				char * suffix = 0;
				switch(txt[0]) {
					case '-': prefix = "--- ";  suffix = " ---"; break;
					case '>': prefix = "  -> "; break;
					case '<': prefix = "  >> "; suffix = " <<"; break;
					case '!': prefix = "!!! ";  suffix = " !!!"; break;
					case '_': prefix = "    "; break;
					case '#': prefix = "  # "; break;
					case '*': prefix = "  * "; break;
					default: txt.prepend("    ");
				};
				if (prefix || suffix) {
					txt.erase(0,1);
					if (prefix) txt.prepend(prefix);
					if (suffix) txt.append(suffix);
				}
			}

			String place;
			if (module) {
				place += "::";
				place += module;
			}
			if (where) {
				place += "::";
				place += where;
			}

			if (!IMfinished) fprintf(Debug::logFile , "\n");

			fprintf(Debug::logFile , "[%s:%03d]%s ## [%s%s%s] \t %s\n"
				, Time64(true).strftime("%M:%S").c_str()
				, GetTickCount() % 1000
				, Debug::logIndent().c_str()
				, this->_plugin.getSig().c_str()
				, place.c_str()
				, thread.c_str()
				, txt.a_str()
				);
			fflush(Debug::logFile);

		}

		Debug::debugLogMsg(this->_plugin, level, module, where, msg);


	}




};  // namespace Konnekt


