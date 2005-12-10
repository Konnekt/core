#include "stdafx.h"
#include "imessage.h"
#include "threads.h"
#include "konnekt_sdk.h"
#include "plugins.h"
#include "debug.h"

using namespace Konnekt::Debug;

namespace Konnekt {

	int dispatchIMessage(sIMessage_base * msg) {
		// Rozsyla wiadomosci
		int result ,  pos;
		result=0;
		if (msg->id==0) {TLSU().setError(IMERROR_UNSUPPORTEDMSG);goto imp_return;}
		if (msg->id == IMC_LOG) {
			plugins[msg->sender].getLogger()->logMsg(logLog, 0, 0, (char*)(static_cast<sIMessage_2params*>(msg)->p1));
			return 0;
		}
		if (msg->id < IMI_BASE) {

#ifdef __DEBUG
			int pos = logIMessage(msg,0,0);
			result = IMCore(msg);
			logIMessageResult(msg, pos, result);
#else
			result=IMCore(msg);
#endif
			if (msg->id != IMC_LOG) {
				TLSU().lastIM.leaveMsg();
			} else 
				TLSU().lastIM.msg = 0;  
		} else
			if (msg->id < IM_BASE || msg->type==0) {
				result = plugins[pluginUI].sendIMessage(msg);
			}
			else
				//  if (msg->id >= IM_SHARE || (msg->sender==0 || msg->sender==ui_sender))
			{
				if (msg->net!=NET_BROADCAST) {
					pos=BCStart-1;
					while ((pos=plugins.findPlugin(msg->net , msg->type , pos+1))>=BCStart)
					{ // Znajduje pierwszy plugin ktory obsluzy wiadomosc
						result = plugins[pos].sendIMessage(msg);
						if (!TLSU().error.code) break;
					}
				} else {
					//BroadCast
#ifdef __DEBUG
					int deb = logIMessage(msg,1+BCStart,0);
#endif
					pos=BCStart-1;
					while ((pos=plugins.findPlugin(msg->net , msg->type , pos+1))>=BCStart)
						plugins[pos].sendIMessage(msg);
#ifdef __DEBUG
					logIMessageResult(msg,deb , 0,1);
#endif
				}
			}/* else {
			 IMBadSender(msg);
			 }
			 */

			//  IMDebug(id,net,type,p1,p2,sender,rcvr,result);
imp_return:
			/*
			if (msg->id != IMC_LOG) {
			TLSU().lastIM.leaveMsg();
			//   lastIM.plugID = msg->sender;
			} else 
			TLSU().lastIM.msg = 0;  
			*/
			return result;
	}

// --------------------------------------------------------
// Plugin	

	int Plugin::sendIMessage(sIMessage_base*im) {

		if (this->_running == false && im->id != IM_PLUG_DEINIT) {
			TLSU().setError(IMERROR_BADPLUG);
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

	IMStackItemRecall::IMStackItemRecall(IMStackItem& item, bool waiting):IMStackItem(item._msg, item._receiver) {
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
		recall->_returnValue = plugins[recall->_receiver].sendIMessage(recall->_msg);
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
			this->error.code = 0;
		}

		this->_stack.pop_back();
	}

	int IMStack::recallThreadSafe(HANDLE thread, bool wait) {
		if (!thread) thread = mainThread.getHandle();
		S_ASSERT(this->getCurrent() != 0);

		IMStackItemRecall* recall = new IMStackItemRecall(*this->getCurrent(), wait);

		if (wait) {
			this->getCurrent()->getMsg()->flag = this->getCurrent()->getMsg()->flag | imfRecalling;
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


};