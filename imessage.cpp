#include "stdafx.h"
#include "imessage.h"
#include "threads.h"
#include "konnekt_sdk.h"
#include "plugins.h"
#include "debug.h"

using namespace Konnekt::Debug;

namespace Konnekt {

	int processIMessage(sIMessage_base * msg , int BCStart=0) {
		// Rozsyla wiadomosci
		int result ,  pos;
		result=0;
		if (msg->id==0) {TLSU().setError(IMERROR_UNSUPPORTEDMSG);goto imp_return;}
		if (msg->id < IMI_BASE) {

#ifdef __DEBUG
			int pos=IMDebug(msg,0,0);
			result = IMCore(msg);
			IMDebugResult(msg,pos,result);
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
					int deb=IMDebug(msg,1+BCStart,0);
#endif
					pos=BCStart-1;
					while ((pos=plugins.findPlugin(msg->net , msg->type , pos+1))>=BCStart)
						plugins[pos].sendIMessage(msg);
#ifdef __DEBUG
					IMDebugResult(msg,deb , 0,1);
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



	int Plugin::sendIMessage(sIMessage_base*im) {

		if (this->_running == false && im->id != IM_PLUG_DEINIT) {
			TLSU().setError(IMERROR_BADPLUG);
			return 0;
		}
		TLSU().lastIM.enterMsg(im , this->_id);
		//  lastIM.receiver =
#ifdef __DEBUG
		int pos = IMDebug(im, this->_id, 0);
#endif
		int result = this->callIMessageProc(im);
#ifdef __DEBUG
		IMDebugResult(im, pos, result);
#endif
		//  lastIM.plugID = msg->sender;
		TLSU().lastIM.leaveMsg();
		return result;

	}

};