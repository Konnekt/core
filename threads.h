#pragma once

#include <Stamina\Thread.h>
#include <Stamina\TLS.h>

#include "konnekt_sdk.h"
#include "main.h"
#include "imessage.h"

namespace Konnekt {

	extern Stamina::Thread mainThread;


	class DLLTempBuffer {
	public:

		String& getString(bool getNew) {
			if (getNew) {
				_current = (_current+1) % _pool.size();
			}
			String& s = _pool[_current];
			return s;
		}

		void * getBuffer(unsigned int wantSize) {
			String& s = this->getString(wantSize != 0);
			return s.useBuffer<char>(wantSize);
		}

		void setPoolSize(int newSize) {
			_pool.resize(newSize);
		}
		int getPoolSize() {
			return _pool.size();
		}

		DLLTempBuffer() {
			this->setPoolSize(poolInitSize);
			_current = 0;
		}
		~DLLTempBuffer() {}
	private:
		std::vector<Stamina::String> _pool;
		int _current;
		const static int poolInitSize = 6;

	};

	class UserThread {
	public:
		COLORREF color;
		IMStack stack;

		const class ThreadInfo* getInfo() const {
			return _info;
		}

		StringRef getName();

		DLLTempBuffer& buffer(unsigned int id = 0) {
			return this->_buffers[id];
		}

		UserThread();
		~UserThread();

	private:
		map <unsigned int , DLLTempBuffer> _buffers; 
		class ThreadInfo * _info;

	};


	extern Stamina::ThreadLocalStorage<UserThread> TLSU;

	class ThreadInfo {
	public:
		ThreadInfo() {
			data = 0;
		}
		Stamina::Thread thread;
		UserThread* data;
		std::string name;
	};

	typedef map <DWORD , ThreadInfo> tThreads;
	extern tThreads threads;
	extern Stamina::CriticalSection threadsCS;


	extern Stamina::oThreadRunner threadRunner;

};