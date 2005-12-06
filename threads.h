#pragma once

#include "konnekt_sdk.h"
#include "main.h"

#include <Stamina\Thread.h>
#include <Stamina\TLS.h>

namespace Konnekt {

	extern Stamina::Thread mainThread;

	struct sIMTS {
		unsigned int msgID;
		unsigned int receiver;
		int retVal;
		HANDLE waitEvent;
		sIMessage_base * msg;
		//     sIMessage temp;
		//     char temp [2000];
		int inMessage; // czy jest w wiadomosci
		int plugID; // identyfikator wtyczki odbierajacej . 0 - Core
#if defined(__DEBUG)
		struct {int id , p1 , p2 , size , sender , rcvr;} debug;
#endif

		void set (sIMessage_base * _msg);
		void enterMsg (sIMessage_base * _msg , unsigned int _plugID);
		void leaveMsg ();
		sIMTS() {msg = 0; inMessage=0; msgID = 0; receiver = 0; retVal = 0; waitEvent = 0; plugID = 0;}
		~sIMTS() {}
		class cUserThread * Thread;
	};

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

	class cUserThread {
	public:
		sIMTS lastIM;
		COLORREF color;
		CStdString name;

		struct {
			int code; 
			int position;
		} error;

		DLLTempBuffer& buffer(unsigned int id = 0) {
			return this->_buffers[id];
		}

		void setError(int code) {
			error.code = code; 
			error.position=lastIM.inMessage;
		}
		cUserThread();
		~cUserThread();

	private:
		map <unsigned int , DLLTempBuffer> _buffers; 

	};


	extern Stamina::ThreadLocalStorage<cUserThread> TLSU;
	typedef map <DWORD , HANDLE> tThreads;
	extern tThreads threads;
	extern Stamina::CriticalSection threadsCS;

	VOID CALLBACK IMTSRecaller(ULONG_PTR param);
	int RecallIMTS(sIMTS & _im , HANDLE thread , bool wait);

	extern Stamina::oThreadRunner threadRunner;

};