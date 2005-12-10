#include "stdafx.h"
#include <Stamina/Image.h>
#include <Stamina/TLS.h>
#include "threads.h"
#include "main.h"
#include "debug.h"
#include "imessage.h"
#include "plugins.h"

using namespace Stamina;

namespace Konnekt {

	ThreadLocalStorage<UserThread> TLSU;
	tThreads threads;
	Stamina::CriticalSection threadsCS;

	Stamina::Thread mainThread;

	class __initializer {
	public:
		__initializer() {
			threads[GetCurrentThreadId()] = mainThread.getHandle();
			threads[GetCurrentThreadId()].name = "Main";
		}
	} _initializer;

	uintptr_t konnektBeginThread (const char*name, void * sec, unsigned stack,	Ctrl::fBeginThread cb, void * args, unsigned flag, unsigned * addr) {
		return (uintptr_t)Ctrl->BeginThread(name, sec, stack, cb, args, flag, addr);
	}

	Stamina::oThreadRunner threadRunner = new Stamina::ThreadRunner(konnektBeginThread);


	int userThreadCount = 0;

	UserThread::UserThread() {
		LockerCS l(threadsCS);
		threads[GetCurrentThreadId()].data = this;
		error.code=0;
		error.position=0;
		lastIM.Thread=this;
		//int v = 0xFFFFFF - ((userThreadCount) * 0x80);
		//v = max(0x505050, v);
		//v = min(0xffffff, v);
		color = getUniqueColor(userThreadCount, 5, 0x80, true);
		userThreadCount++;
	}
	UserThread::~UserThread() {
		userThreadCount --;
	}




};