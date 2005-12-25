#include "stdafx.h"
#include "konnekt_sdk.h"
#include "threads.h"
#include "plugins.h"
#include "debug.h"
#include "imessage.h"

using namespace Konnekt::Debug;

namespace Konnekt {

	int dispatchIMessage(sIMessage_base * msg) {

		using namespace Net;

		if (msg->id == IMC_LOG) {
			plugins[msg->sender].getLogger()->logMsg(logLog, 0, 0, (char*)(static_cast<sIMessage_2params*>(msg)->p1));
			return 0;
		}

		UserThread& thread = TLSU();

		if (msg->id < IM_BASE) { // komunikaty do wtyczki CORE / UI
			return plugins[ msg->id < IMI_BASE ? pluginCore : pluginUI ].sendIMessage(msg);
		} else {
			// broadcast...
			Net::Broadcast net (msg->net);

			bool reverse = net.isReverse();
			int startPos = plugins.getIndex( net.getStartPlugin() ); // zawsze sprawdza zakres
			if (startPos == pluginNotFound) {
				thread.stack.setError(errorBadBroadcast);
				return 0;
			}
			// przy odwróconej kolejnoœci, startPlugin je¿eli jest pozycj¹, jest pozycj¹ od koñca!
			if (reverse && net.getStartPlugin() <= pluginsMaxCount) {
				startPos = plugins.count() - startPos - 1;
			}
			Plugins::tList::iterator it = plugins.begin() + startPos;
			// na przysz³oœæ, dla szybszych porównañ...
			Plugins::tList::iterator end = reverse ? plugins.begin() : plugins.end();

			// szykujemy dane, ¿eby nie wyci¹gaæ ich za ka¿dym razem...
			int occurence = net.getStartOccurence();
			tNet onlyNet = net.getOnlyNet();
			tNet notNet = net.getNotNet();

			if (msg->type == imtNone) msg->type = imtAll; // poprawiamy potencjalny problem

			int result = 0;

			if (Debug::logAll && net.isSpecial()) {
				logIMessageBC(msg);
			}

			int hits = 0;

			do {
				Plugin& plugin = *(*it);
				if (onlyNet != Net::all && onlyNet != plugin.getNet()) continue;
				if (notNet != Net::all && notNet == plugin.getNet()) continue;
				if (msg->type != imtAll) {
					switch (net.getIMType()) {
						case Broadcast::imtypeAny: 
							if ((msg->type & plugin.getType()) != 0) {break;} continue;
						case Broadcast::imtypeNot: 
							if ((msg->type & plugin.getType()) == 0) {break;} continue;
						default: /*all*/
							if ((msg->type & plugin.getType()) == msg->type) {break;} continue;
					};
				}
				// passed!
				if (occurence - hits > 0) continue;
				// passed again! wysy³amy...

				int retVal = plugin.sendIMessage(msg);

				if (thread.stack.getError() != errorNoResult) {
					hits++;
					switch (net.getResultType()) {
						case Broadcast::resultAnd:
							result = (hits == 1) ? retVal : result & retVal;
							break;
						case Broadcast::resultOr:
							result |= retVal;
							break;
						case Broadcast::resultSum:
							result += retVal;
							break;
						default: // last
							result = retVal;
					}
				}
			
			} while ( reverse ? ( (it--) != end ) : ( (++it) != end ) ); // przy cofaniu, sprawdzamy czy obecnie przerobiony nie jest ostatnim... W normalnym zawsze mamy zapas...

			if (Debug::logAll && net.isSpecial()) {
				logIMessageBCResult(msg, result, hits);
			}

			return result;
		}
	}

// --------------------------------------------------------
// Plugin	

	int Plugin::sendIMessage(sIMessage_base*im) {

		if (this->_running == false && im->id != IM_PLUG_DEINIT && this->_ctrl != 0) {
			TLSU().stack.setError(IMERROR_BADPLUG);
			return 0;
		}
		TLSU().stack.pushMsg(im, *this);
		//  lastIM.receiver =
#ifdef __DEBUG
		int pos = logIMessage(im, *this);
#endif
		int result = this->callIMessageProc(im);
#ifdef __DEBUG
		logIMessageResult(im, pos, result);
#endif
		//  lastIM.plugID = msg->sender;
		TLSU().stack.popMsg();
		return result;

	}

// --------------------------------------------------------
// IMStack	

	IMStackItemRecall::IMStackItemRecall(IMStackItem& item, bool waiting):IMStackItem(item.getMsg(), item.getReceiver()) {
		if (waiting == false) {
			_waitEvent = 0;
			void * temp = malloc(_msg->s_size);
			memcpy(temp, _msg, _msg->s_size);
			_msg = (sIMessage_base*)temp;
		} else {
			_returnValue = 0;
			_waitEvent = new Stamina::Event();
		}
	}

	IMStackItemRecall::~IMStackItemRecall() {
		if (isWaiting() == false) free(_msg);
		if (_waitEvent) delete _waitEvent;
	}

	VOID CALLBACK IMStackItemRecall::threadRecaller(ULONG_PTR param) {
		IMStackItemRecall* recall = (IMStackItemRecall*)param;
		recall->_msg->flag = recall->_msg->flag | imfRecalled;

		mainLogger->log(logDebug, 0, 0, "<T=%x", recall->getMsg()->id);
		recall->_returnValue = recall->_receiver.sendIMessage(recall->_msg);
		if (recall->isWaiting()) {
			recall->_waitEvent->pulse();
		} else {
			delete recall;
		}
	}





	void IMStack::pushMsg(sIMessage_base* msg, Plugin& receiver) {

		this->error.code = errorNone;
		this->_stack.push_back(IMStackItem(msg, receiver));

	}

	void IMStack::popMsg() {
		if (this->error.code != errorNone && this->error.position != this->count()) {
			this->error.code = errorNone;
		}

		this->_stack.pop_back();
	}

	int IMStack::recallThreadSafe(HANDLE thread, bool wait, IMStackItem* item) {
		if (!thread) thread = mainThread.getHandle();
		S_ASSERT(this->getCurrent() != 0);

		if (!item) {
			item = this->getCurrent();
		}

		IMStackItemRecall* recall = new IMStackItemRecall(*item, wait);

		if (wait) {
			item->getMsg()->flag = item->getMsg()->flag | imfRecalling;
		}

		mainLogger->log(logDebug, 0, 0, ">T=%x %s", recall->getMsg()->id ,  wait ? "" : " NW");

		QueueUserAPC(IMStackItemRecall::threadRecaller, thread, (ULONG_PTR)recall);

		if (wait) {
			recall->wait();
			int r = recall->getReturnValue();
			delete recall;
			return r;
		} else {
			this->setError(errorThreadSafe);
			
			return 0;
		}
	}


	String IMStack::debugInfo() const {
		String msg;
		// Wypisuje informacje o aktualnym/ostatnim IMessage
		if (!running || this->count() == 0) {
			msg = "(none)";
		} else {
			for (tStack::const_reverse_iterator it = _stack.rbegin(); it != _stack.rend(); ++it) {
				const IMStackItem* im = &(*it);
				IMessageInfo info(im->getMsg());
				msg += stringf("%s%s[%x/%x]%s -> %s | %s | %s | %s\n"
					, (im->getMsg()->flag & imfRecalling) ? ">w" : ""
					, (im->getMsg()->flag & imfRecalled) ? ">>" : ""
					, inttostr( im->getNumber() ).c_str()
					, inttostr( im->getTicks() ).c_str()
					, info.getPlugin(im->getMsg()->sender).c_str() 
					, info.getPlugin(im->getReceiver()).c_str()
					, info.getNet().c_str()
					, info.getId().c_str()
					, info.getData().c_str()
					);
			}
		}

		return PassStringRef(msg);
	}


};