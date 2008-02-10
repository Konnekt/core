#include "stdafx.h"
#include <Stamina/UI/Image.h>
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
			threads[GetCurrentThreadId()].thread = mainThread;
			threads[GetCurrentThreadId()].name = "Main";
		}
	} _initializer;

	uintptr_t konnektBeginThread (const char*name, void * sec, unsigned stack,	Controler::fBeginThread cb, void * args, unsigned flag, unsigned * addr) {
		return (uintptr_t)Ctrl->BeginThread(name, sec, stack, cb, args, flag, addr);
	}

	Stamina::oThreadRunner threadRunner = new Stamina::ThreadRunner(konnektBeginThread);


	int userThreadCount = 0;

	UserThread::UserThread() {
		LockerCS l(threadsCS);
		this->_info = &threads[GetCurrentThreadId()];
		this->_info->data = this;
		color = Stamina::UI::getUniqueColor(userThreadCount, 5, 0x80, true);
		userThreadCount++;
	}
	UserThread::~UserThread() {
		userThreadCount --;
	}


	StringRef UserThread::getName() {
		return this->_info->name;
	}

	void UserThread::setName(const StringRef& name) {
		if (this->_info) {
			this->_info->name = name;
		}
	}



};