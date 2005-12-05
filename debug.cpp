#include "stdafx.h"
#include "debug.h"
#include "beta.h"
#include "plugins.h"
#include "tables.h"
#include "imessage.h"
#include "threads.h"
#include "argv.h"

#define LOG_TAB 9

using namespace Stamina;

namespace Konnekt { namespace Debug {

	bool superUser=false;
	CStdString logPath;
	CStdString logFileName; // nazwa pliku z logiem
	FILE * logFile;
	int IMlogsize = 0;
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
						unlink(file);
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
		Debug::debugAll = getArgV(ARGV_DEBUGALL);
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







	// __declspec (thread) char imclog_buff [20000];

	int IMDebug_transform(sIMDebug & IMD , sIMessage_base * msgBase , int result , int error) {
		sIMessage_2params * msg=0;
		sIMessage_2params msgCopy;
		if (!msgBase) {
			IMD.result = "0x" + inttostr(result , 16);
			IMD.error = inttostr(error);
			IMD.thread = inttostr(GetCurrentThreadId() , 26);
			return 0;
		} else {
			if (msgBase->s_size >= sizeof(sIMessage_2params)) {
				msg = static_cast<sIMessage_2params*>(msgBase);
			} else {
				memcpy(&msgCopy , msgBase , sizeof(sIMessage_base));
				msgCopy.p1 = msgCopy.p2 = 0;
				msg = &msgCopy;
			}
			IMD.id = inttostr(msg->id);
			IMD.p1 = "0x" + inttostr(msg->p1 , 16);
			IMD.p2 = "0x" + inttostr(msg->p2 , 16);
			IMD.nr=inttostr(IMlogsize);
			IMD.sender=Plug.Name(msg->sender);
			IMD.net= (msg->net==NET_BC)?"BC" : inttostr(msg->net);
			IMD.type=inttostr(msg->type,2,12);
		}  
		cMessage * m;
		char * ch;
		switch (msg->id) {
		case IMC_LOG :
			if (!msg->p1) return 0;
			IMD.id = "IMC_LOG";
			ch = (char*)msg->p1;
			//             if (ch[strlen(ch)-1]=='\n') ch[strlen(ch)-1]=0;
			if (ch[0] && ch[1] && ch[0] != ch[1]) {
				switch(ch[0]) {
					case '-': IMD.p1 = "--- " + string(ch+1) + " ---";break;
					case '>': IMD.p1 = "  -> " + string(ch+1);break;
					case '<': IMD.p1 = "  >> " + string(ch+1) + " <<";break;
					case '!': IMD.p1 = "!!! " + string(ch+1) + " !!!";break;
					case '_': IMD.p1 = "    " + string(ch+1);break;
					case '#': IMD.p1 = "  # " + string(ch+1);break;
					case '*': IMD.p1 = "  * " + string(ch+1);break;
					default: IMD.p1 = "   " + string(ch);
				}
			} else {
				IMD.p1 = ch;
			}
			IMD.p2="";
			break;
#ifdef __WITHDEBUGALL
		case IMC_THREADSTART:IMD.id="IMC_THREADSTART"; break;
		case IMC_THREADEND:IMD.id="IMC_THREADEND"; break;
		case IM_THREADSTART:IMD.id="IM_THREADSTART"; break;
		case IM_THREADEND:IMD.id="IM_THREADEND"; break;

		case IM_PLUG_INIT:IMD.id="IM_PLUG_INIT"; break;

		case IMI_WARNING :
			IMD.id = "IMI_WARNING";
			if (!msg->p1) return 0;
			stringf(IMD.p1.c_str() , "\"%.2000s\"" , msg->p1);
			break;
		case IMI_ERROR :
			IMD.id = "IMI_ERROR";
			if (!msg->p1) return 0;
			stringf(IMD.p1.c_str() , "\"%.2000s\"" , msg->p1);
			break;
		case IMI_PREPARE :
			IMD.id = "IMI_PREPARE";
			break;
			//        case IMI_UIINVALIDATE :
			//             sprintf(IMD.id , "IMI_UIINVALIDATE");
			//             break;
		case IMI_ACTION :
			IMD.id = "IMI_ACTION";
			if (!msg->p1 || !msg->p2) return 0;
			stringf(IMD.p1.c_str() , "%d / %d / %x" , ((sUIActionInfo*)msg->p1)->act.parent,((sUIActionInfo*)msg->p1)->act.id);
			stringf(IMD.p2.c_str() , "\"%.2000s\"" , ((sUIActionInfo*)msg->p1)->txt);
			break;
		case IMI_ACTION_SET :
			IMD.id = "IMI_ACTION_SET";
			if (!msg->p1) return 0;
			stringf(IMD.p1.c_str() , "%d / %d / %x m=%x" , ((sUIActionInfo*)msg->p1)->act.parent,((sUIActionInfo*)msg->p1)->act.id , ((sUIActionInfo*)msg->p1)->act.cnt , ((sUIActionInfo*)msg->p1)->mask);
			break;	 

		case IMI_REFRESH_LST :
			IMD.id = "IMI_REFRESH_LST";
			break;
		case IMI_REFRESH_CNT :
			IMD.id = "IMI_REFRESH_CNT";
			break;
		case IMI_NOTIFY :
			IMD.id = "IMI_NOTIFY";
			break;
		case IMI_NEWNOTIFY :
			IMD.id = "IMI_NEWNOTIFY";
			break;

		case IMC_MESSAGENOTIFY :
			IMD.id = "IMC_MESSAGENOTIFY";
			break;
		case IMC_SHUTDOWN :
			IMD.id = "IMC_SHUTDOWN";
			break;
		case IMC_NEWMESSAGE :
			IMD.id = "IMC_NEWMESSAGE";
			if (!msg->p1) return 0;
			m = (cMessage *) msg->p1;
			stringf(IMD.p1.c_str() , "id=%d net=%d from[\"%.50s\"] to[\"%.50s\"]" , m->id , m->net , m->fromUid , m->toUid);
			break;
		case IMC_MESSAGEQUEUE :
			IMD.id = "IMC_MESSAGEQUEUE";
			break;
		case IMC_MESSAGEWAITING :
			IMD.id = "IMC_MESSAGEWAITING";
			break;
			/*        case IMC_MESSAGEPOP :
			IMD.id = "IMC_MESSAGEPOP");
			break;*/
		case IMC_MESSAGEREMOVE :
			IMD.id = "IMC_MESSAGEREMOVE";
			break;
		case IMC_CNT_IGNORED :
			IMD.id = "IMC_CNT_IGNORED";
			if (!msg->p2) return 0;
			stringf(IMD.p2.c_str() , "\"%.2000s\"" , msg->p2);
			break;
		case IMC_FINDCONTACT :
			IMD.id = "IMC_FINDCONTACT";
			if (!msg->p2) return 0;
			stringf(IMD.p2.c_str() , "\"%.2000s\"" , msg->p2);
			break;
		case IMC_PROFILEDIR :
			IMD.id = "IMC_PROFILEDIR";
			if (!result) return 0;
			stringf(IMD.result , "%.2000s" , result);
			break;
		case IMC_CNT_REMOVE :
			IMD.id = "IMC_CNT_REMOVE";
			break;
		case IMC_CFG_SETCOL : case IMC_CNT_SETCOL : {
			sSETCOL * sc = (sSETCOL*)msg->p1;
			IMD.id = msgBase->id == IMC_CFG_SETCOL ? "IMC_CFG_SETCOL" : "IMC_CNT_SETCOL";
			stringf(IMD.p1 , "id=%d type=%d def=%d name=%s" , sc->id , sc->type , sc->def , sc->name);
			break;
							  }
		case sIMessage_setColumn::__msgID: {
			sIMessage_setColumn * sc = (sIMessage_setColumn*)msgBase;
			stringf(IMD.p1 , "table=%d id=%d type=%d def=%d name=%s" , sc->_table , sc->_id , sc->_type , sc->_def , sc->_name);
			break;
										   }
		case IMI_MSG_OPEN :
			IMD.id = "IMI_MSG_OPEN";
			break;
		case IM_START :
			IMD.id = "IM_START";
			break;
		case IM_END :
			IMD.id = "IM_END";
			break;
		case IM_UIACTION :
			IMD.id = "IM_UIACTION";
			if (!msg->p1) return 0;
			stringf(IMD.p1 , "%d / %d / %x = %d" , ((sUIActionNotify_base*)msg->p1)->act.parent,((sUIActionNotify_base*)msg->p1)->act.id , ((sUIActionNotify_base*)msg->p1)->act.cnt , ((sUIActionNotify_base*)msg->p1)->code);
			break;	 
		case IM_CONNECT :
			IMD.id = "IM_CONNECT";
			break;
		case IM_UI_PREPARE :
			IMD.id = "IM_UI_PREPARE";
			break;
		case IM_MSG_OPEN :
			IMD.id = "IM_MSG_OPEN";
			break;
		case IM_MSG_SEND :
			IMD.id = "IM_MSG_SEND";
			if (!msg->p1) return 0;
			m = (cMessage *) msg->p1;
			stringf(IMD.p1 , "id=%d net=%d to[\"%.50s\"]" , m->id , m->net , m->toUid);
			break;
		case IM_MSG_RCV :
			IMD.id = "IM_MSG_RCV";
			break;
		case IM_CNT_STATUSCHANGE :
			IMD.id = "IM_CNT_STATUSCHANGE";
			break;

			/*        case IMC_GETCNTC :
			IMD.id = "IMC_GETCNTC");
			sprintf(IMD.result , "\"%s\""  , safeChar(result));
			break;
			case IMC_SETCNTC :
			IMD.id = "IMC_SETCNTC");
			sprintf(IMD.p2 , "%d | \"%s\""  , ((sSETCNT*)p2)->typ , safeChar(((sSETCNT*)p2)->val));
			break;
			case IMC_SETCNTI :
			IMD.id = "IMC_SETCNTI");
			sprintf(IMD.p2 , "%d | %x & %x"  , ((sSETCNT*)p2)->typ , safeChar(((sSETCNT*)p2)->val) , ((sSETCNT*)p2)->mask);
			break;
			*/             
		case IMC_IGN_DEL :
			IMD.id = "IMC_IGN_DEL";
			if (!msg->p1 || !msg->p2) return 0;
			stringf(IMD.p1 , "%d@%.50s"  , msg->p1,msg->p2);
			break;
		case IM_AWAY :
			IMD.id = "IM_AWAY";
			break;
		case IM_BACK :
			IMD.id = "IM_BACK";
			break;
#endif
		}

		return 1;
	}


	int IMDebug(sIMessage_base * msg , unsigned int rcvr , int result) {
		if (!debug || Debug::logFile == 0) return -1;
		if (msg->id != IMC_LOG && !Debug::debugAll) return -1;
		if (msg->id == IMC_THREADEND || msg->id == IMC_THREADSTART)
			return -1;
//		static cCriticalSection_WM CSection(WMProcess2);
		static CriticalSection CSection;
		//if (debugLock) CSection.lock();
		//  if (hMainThread && (GetCurrentThreadId()!=MainThreadID)) {
		//     Beep(200,20);//return 0;
		//  }
		static bool inCritical = false;
		/*  if (inCritical) {
		Beep(1000,10);
		}*/
		//EnterCriticalSection(&section_debug);
		inCritical = true;

		CSection.lock();

		//cThread thread_copy = TLS();
		sIMDebug IMD;
		IMDebug_transform(IMD , msg,result,TLSU().error.code);
		IMD.receiver=Plug.Name(rcvr);
		string ind;
		ind.resize(indent[GetCurrentThreadId()] , LOG_TAB);
		if (Debug::logFile) {
			if (!IMfinished) fprintf(Debug::logFile , "\n");


			static Stamina::Time64 lastLog(false);
			Stamina::Time64 now(true);
			// co 5 minut, lub o ka¿dej pe³nej godzinie wstawia pe³n¹ datê
			if (now - lastLog > 60*5 || (int)(lastLog / 86400) < (int)(now / 86400)) {
				fprintf(Debug::logFile, "[%s] ------------ ", now.strftime("%c").c_str());
			}
			lastLog = now;


#ifdef __LOGFULL
			fprintf(Debug::logFile , "%s[%s:%03d][%s] %s -> %s\t | %s\t | %s %s\t | %s %s"
				, ind.c_str() , now.strftime("%M:%S").c_str(), GetTickCount() / 1000, IMD.nr.c_str() , IMD.sender.c_str() , IMD.receiver.c_str()
				, IMD.id.c_str() , IMD.net.c_str() , IMD.type.c_str()
				, IMD.p1.c_str() , IMD.p2.c_str()
				);
#else
			if (msg->id == IMC_LOG)
				fprintf(Debug::logFile , "%s[%s:%03d]## [%s] \t %s"
				, ind.c_str() , now.strftime("%M:%S").c_str(), GetTickCount() / 1000, IMD.sender.c_str()
				, IMD.p1.c_str()
				);
			else
				fprintf(Debug::logFile , "%s[%s] %s -> %s\t | %s\t | %s %s "
				, ind.c_str() , IMD.nr.c_str() , IMD.sender.c_str() , IMD.receiver.c_str()
				, IMD.id.c_str() , IMD.p1.c_str() , IMD.p2.c_str()
				);
#endif
			fflush(Debug::logFile);
		}

		indent[GetCurrentThreadId()] ++;
		//TLS() = thread_copy;
		CSection.unlock();

		debugLogStart(IMD , msg , ind);
		IMfinished = false;
		IMlogsize ++;
		//LeaveCriticalSection(&section_debug);
		//if (debugLock) CSection.unlock();
		inCritical=false;
		return IMlogsize-1;
	}

	int IMDebugResult(sIMessage_base * msg , int pos , int res , int BC) {
		if (!debug || Debug::logFile == 0) return 0;
		if (msg->id != IMC_LOG && msg->id && !Debug::debugAll) return 0;
		if (msg->id == IMC_THREADEND || msg->id == IMC_THREADSTART)
			return 0;
		if (pos<0) return 0;
		/*  if (hMainThread && (GetCurrentThreadId()!=MainThreadID)) {
		Beep(800,20);return 0;
		}*/
		static bool inCritical = false;
		static CriticalSection_WM CSection(mainWindowsLoop);
		if (debugLock) CSection.lock();
		/*  if (inCritical) {
		Beep(1000,10);
		}*/
		//EnterCriticalSection(&section_debug);
		inCritical = true;
		//cThread thread_copy = TLS();
		sIMDebug IMD;
		string ind;
		ind.resize(indent[GetCurrentThreadId()] , LOG_TAB);
		IMDebug_transform(IMD , 0,res,TLSU().error.code);

		if (pos == IMlogsize-1) {
			debugLogEnd(IMD , msg , false , ind);
			if (msg->id == IMC_LOG)
				fprintf(Debug::logFile , "\n");
			else
				fprintf(Debug::logFile , "\t= %s %s t%s\n" , IMD.result.c_str() , TLSU().error.code?(char*)IMD.error.c_str():"" , IMD.thread.c_str());
			indent[GetCurrentThreadId()] --;
			IMfinished = true;
		} else {
			indent[GetCurrentThreadId()] --;
			ind.resize(indent[GetCurrentThreadId()] , LOG_TAB);
			IMfinished = true;
			debugLogEnd(IMD , msg , true , ind);
			if (Debug::logFile) {
				fprintf(Debug::logFile , "%s= %s\n" , ind.c_str() , IMD.result.c_str());
				fflush(Debug::logFile);
			}
		}
		//TLS() = thread_copy;
		//LeaveCriticalSection(&section_debug);
		inCritical = false;
		if (debugLock) CSection.unlock();
		return 0;
	}

#endif




};};
