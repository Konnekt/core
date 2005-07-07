#include "stdafx.h"
#include "threads.h"
#include "main.h"
#include "debug.h"
#include "imessage.h"
#include "plugins.h"

namespace Konnekt {

	cTLS<cUserThread> TLSU;
	tThreads threads;
	Stamina::CriticalSection threadsCS;

	Stamina::Thread mainThread;

	class __initializer {
	public:
		__initializer() {
			threads[GetCurrentThreadId()] = mainThread.getHandle();
		}
	} _initializer;

	uintptr_t konnektBeginThread (const char*name, void * sec, unsigned stack,	cCtrl::fBeginThread cb, void * args, unsigned flag, unsigned * addr) {
		return (uintptr_t)Ctrl->BeginThread(name, sec, stack, cb, args, flag, addr);
	}

	Stamina::oThreadRunner threadRunner = new Stamina::ThreadRunner(konnektBeginThread);

	void sIMTS::enterMsg (sIMessage_base * _msg , unsigned int _plugID) {
		Thread->error.code = 0;
		inMessage++;msg=_msg; plugID=_plugID;
#if defined(__DEBUG)
		debug.size = _msg->s_size;
		debug.id = _msg->id;
		debug.sender = _msg->sender;
		debug.rcvr = _plugID;
		if (debug.size >= sizeof(sIMessage_2params)) {
			debug.p1 = (static_cast<sIMessage_2params*>(_msg))->p1;
			debug.p2 = (static_cast<sIMessage_2params*>(_msg))->p2;
		} else {debug.p1 = debug.p2 = 0;}
#endif
	}
	void sIMTS::leaveMsg() {
		if (Thread->error.code && Thread->error.position != inMessage) {Thread->error.code = 0;}
		inMessage--;
		msg=0;
	}
	void sIMTS::set(struct sIMessage_base * _msg) {
		msg = _msg;
	}
	// *************************************  THREAD SAFE

	VOID CALLBACK IMTSRecaller(ULONG_PTR param) {
		isWaiting = false;
		sIMTS * imts = (sIMTS*)param;
		if (!imts->msg) return;
		//     IMLOG("_ ThreadSafe Recaller plugID=0x%x msg_id=%d" , imts->plugID , imts->msg->id);
		IMLOG("<T=%x" , imts->msg->id);
		if (!imts->plugID) {
			imts->retVal = IMCore(imts->msg);
			TLSU().lastIM.leaveMsg();
		} else {
			imts->retVal = IMessageDirect(Plug[imts->plugID] , imts->msg);
		}
		if (imts->waitEvent) {
			SetEvent(imts->waitEvent);
		} else {free(imts->msg);delete imts;}
	}

	int RecallIMTS(sIMTS & _im , HANDLE thread , bool wait) {
		if (!thread) thread = hMainThread;
		sIMTS * imts = new sIMTS;
		*imts = _im; // Robimy kopie imts'a
		//     IMLOG("_ Thread Safe Switch %s! %s" , wait?"":"NO Wait" , isWaiting?"W":"NW");
		IMLOG(">T=%x %s" , imts->msg->id ,  wait?"":" NW");
		imts->retVal = 0;
		imts->waitEvent = 0;
		if (wait) imts->waitEvent = CreateEvent(0 , 0 , 0 , 0);
		else {
			sIMessage_base * temp = static_cast<sIMessage_base*>(malloc(imts->msg->s_size));     // Robimy kopiê wiadomoœci
			memcpy(temp , imts->msg , imts->msg->s_size);
			imts->msg = temp;
		}
		QueueUserAPC(IMTSRecaller , thread , (ULONG_PTR)imts);
		if (wait) {
			while (WaitForSingleObjectEx(imts->waitEvent , INFINITE , 1)!=WAIT_OBJECT_0) {}
			int r = imts->retVal;
			CloseHandle(imts->waitEvent);
			delete imts;
			return r;
		} else {
			TLSU().setError(IMERROR_THREADSAFE);
			return 0;
		}

	}

	int userThreadCount = 0;

	cUserThread::cUserThread() {
		error.code=0;
		error.position=0;
		lastIM.Thread=this;
		//int v = 0xFFFFFF - ((userThreadCount) * 0x80);
		//v = max(0x505050, v);
		//v = min(0xffffff, v);
		color = getUniqueColor(userThreadCount, 5, 0x80, true);
		OutputDebugString(_sprintf("TCOLOR = %x c=%d\n", color, userThreadCount));
		userThreadCount++;
	}
	cUserThread::~cUserThread() {
		userThreadCount --;
	}




};