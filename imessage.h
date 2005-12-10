
#pragma once

#include <list>
#include "konnekt_sdk.h"


namespace Konnekt {

	struct IMStackItem {
	public:
		IMStackItem(sIMessage_base* msg, Plugin& receiver):_receiver(receiver), _msg(msg) {
		}

		inline sIMessage_base* getMsg() const {
			return _msg;
		}

		inline Plugin& getReceiver() const {
			return _receiver;
		}

	private:
		sIMessage_base* _msg;
		Plugin& _receiver;
	};

	class IMStackItemRecall: public IMStackItem {
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

		
	private:

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

		int recallThreadSafe(HANDLE thread, bool wait);

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