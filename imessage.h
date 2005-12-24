#pragma once

#include <list>
#include "konnekt_sdk.h"
#include "plugins.h"


namespace Konnekt {

	class IMStackItem {
	public:
		IMStackItem(sIMessage_base* msg, Plugin& receiver):_receiver(receiver), _msg(msg) {
			static volatile long imessageNumber = 0;
			_ticks = GetTickCount();
			_number = InterlockedIncrement(&imessageNumber);
		}

		inline sIMessage_base* getMsg() const {
			return _msg;
		}

		inline Plugin& getReceiver() const {
			return _receiver;
		}

		inline unsigned long getTicks() const {
			return _ticks;
		}

		inline unsigned long getNumber() const {
			return _number;
		}

	protected:
		sIMessage_base* _msg;
		Plugin& _receiver;
		unsigned long _ticks;
		unsigned long _number;
	};

	class IMStackItemRecall: public IMStackItem {

		friend class IMStack;

	public:
		IMStackItemRecall(IMStackItem& item, bool waiting);
		~IMStackItemRecall();

		bool isWaiting() {
			return _waitEvent != 0;
		}

		void wait() {
			S_ASSERT(_waitEvent != 0);
			while (_waitEvent->wait(INFINITE, true) != WAIT_OBJECT_0) {}
		}

		inline int getReturnValue() {
			return _returnValue;
		}

		
	protected:

		static VOID CALLBACK threadRecaller(ULONG_PTR param);

		Event* _waitEvent;
		int _returnValue;
	};

	// Stos komunikatów dla w¹tku...
	class IMStack {
	public:

		typedef list<IMStackItem> tStack;

		void pushMsg(sIMessage_base* msg, Plugin& receiver);

		void popMsg();

		int recallThreadSafe(HANDLE thread, bool wait, IMStackItem* recallMsg = 0);

		IMStackItem* getCurrent() {
			if (_stack.size() > 0) {
				return &_stack.back();
			}
			return 0;
		}

		void setError(enIMessageError code) {
			error.code = code; 
			error.position = this->count();
		}

		enIMessageError getError() {
			return error.code;
		}

		inline int count() const {
			return _stack.size();
		}

		String debugInfo() const;

	private:

		tStack _stack;

		struct {
			enIMessageError code; 
			int position;
		} error;


	};





	int coreIMessageProc(sIMessage_base * msgBase);
	int coreDebugCommand(sIMessage_debugCommand * arg);

	int dispatchIMessage(sIMessage_base * msg);

};