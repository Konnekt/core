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

	struct cDLLTempBuffer {
		void * buff; 
		unsigned int size;

		void * GetBuffer(unsigned int wantSize) {
			if (wantSize > this->size) {
				//if (buff) free(buff);
				buff = realloc(buff, wantSize);
				this->size = wantSize;
			}
			return buff;
		}

		cDLLTempBuffer() {buff = 0; size = 0;}
		~cDLLTempBuffer() {if (buff) free(buff);}
	};

	class cUserThread {
	public:
		sIMTS lastIM;
		COLORREF color;
		CStdString name;
		CStdString buff;
		char shortBuffer [64];

		struct {
			int code; 
			int position;
		} error;

		map <unsigned int , cDLLTempBuffer> Buffers; 
		void setError(int code) {
			error.code = code; 
			error.position=lastIM.inMessage;
		}
		cUserThread();
		~cUserThread();
	};


	extern Stamina::ThreadLocalStorage<cUserThread> TLSU;
	typedef map <DWORD , HANDLE> tThreads;
	extern tThreads threads;
	extern Stamina::CriticalSection threadsCS;

	VOID CALLBACK IMTSRecaller(ULONG_PTR param);
	int RecallIMTS(sIMTS & _im , HANDLE thread , bool wait);

	extern Stamina::oThreadRunner threadRunner;

};