#include "stdafx.h"
#include "imessage.h"
#include "threads.h"
#include "konnekt_sdk.h"
#include "plugins.h"
#include "debug.h"

using namespace Konnekt::Debug;

namespace Konnekt {

	void IMBadSender(sIMessage_base * msg , int rcvr) {
		TLSU().setError(IMERROR_BADSENDER);
#ifdef __DEBUG
		IMLOG("! IMERROR_BADSENDER  %s->%s ID=%d NOT ALLOWED" , Plug.Name(msg->sender).c_str() , Plug.Name(rcvr) , msg->id);
#endif
	}

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
				result=IMessageDirect(Plug[0] , msg);
			}
			else
				//  if (msg->id >= IM_SHARE || (msg->sender==0 || msg->sender==ui_sender))
			{
				if (msg->net!=NET_BROADCAST) {
					pos=BCStart-1;
					while ((pos=Plug.Find(msg->net , msg->type , pos+1))>=BCStart)
					{ // Znajduje pierwszy plugin ktory obsluzy wiadomosc
						result=IMessageDirect(Plug[pos],msg);
						if (!TLSU().error.code) break;
					}
				} else {
					//BroadCast
#ifdef __DEBUG
					int deb=IMDebug(msg,1+BCStart,0);
#endif
					pos=BCStart-1;
					while ((pos=Plug.Find(msg->net , msg->type , pos+1))>=BCStart)
						IMessageDirect(Plug[pos],msg);
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
	int * broadcastIMessage(unsigned int id , unsigned int type , int p1 , int p2 , int & count , int BCStart=1) {
		// Zwraca tablice wynikow BroadCast'a
		int * res = new int [MAX_PLUGS];
		count=0;
		int result;
#ifdef __DEBUG
		int deb=IMDebug(&sIMessage(id,-1,type,p1,p2),1+BCStart,0);
#endif
		int pos=BCStart-1;
		while ((pos=Plug.Find(-1 , type , pos+1))>=BCStart)
		{
			//	   cUserThread * ut = &TLSU();
			//   IMLOG("Sum tlsuthred=%x cid=%x %x . ecode=%d epos=%d" , TLSU().lastIM.Thread , GetCurrentThreadId() , &TLSU() , TLSU().error.code , TLSU().error.position);
			result=IMessageDirect(Plug[pos],&sIMessage(id,p1,p2));
			//   IMLOG("Sum pos=%d result=0x%x count=%d ecode=%d epos=%d" , pos , result , count , TLSU().error.code , TLSU().error.position);
			if (!TLSU().error.code) {res[count]=result; count++;}
		}
#ifdef __DEBUG
		IMDebugResult(&sIMessage(id,-1,type,p1,p2),deb , count,1);
#endif
		return res;
	}

	int sumIMessage(unsigned int id , unsigned int type , int p1 , int p2 , int BCStart) {
		int count , sum=0;
		int * res=IMessageBC(id , type , p1 , p2 , count , BCStart);
		for (int i=0;i<count;i++) sum+=res[i];
		delete [] res;
		// Zwraca sume wynikow BroadCast'a
		return sum;
	}

	int directIMessage(cPlug & plug, sIMessage_base * msg){
		if (!plug.running && msg->id != IM_PLUG_DEINIT) {TLSU().setError(IMERROR_BADPLUG);return 0;}
		TLSU().lastIM.enterMsg(msg , plug.ID);
		//  lastIM.receiver =
#ifdef __DEBUG
		int pos=IMDebug(msg,plug.ID,0);
#endif
		int result = plug.IMessageProc(msg);
#ifdef __DEBUG
		IMDebugResult(msg,pos,result);
#endif
		//  lastIM.plugID = msg->sender;
		TLSU().lastIM.leaveMsg();
		return result;
	}

};