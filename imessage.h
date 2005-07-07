
#pragma once

#include "konnekt_sdk.h"

namespace Konnekt {

	int coreIMessageProc(sIMessage_base * msgBase);
	int coreDebugCommand(sIMessage_debugCommand * arg);
	inline int IMCore(sIMessage_base * msgBase) {
		return coreIMessageProc(msgBase);
	}

	int processIMessage(sIMessage_base * msg , int);
	inline int IMessageProcess(sIMessage_base * msg , int start) {
		return processIMessage(msg, start);
	}
	int * broadcastIMessage(unsigned int id , unsigned int type , int p1 , int p2 , int & count , int BCStart);
	inline int * IMessageBC(unsigned int id , unsigned int type , int p1 , int p2 , int & count , int BCStart) {
		return broadcastIMessage(id, type, p1, p2, count, BCStart);
	}
	int sumIMessage(unsigned int id , unsigned int type=0 , int p1=0 , int p2=0 , int BCStart=0);
	inline int IMessageSum(unsigned int id , unsigned int type=0 , int p1=0 , int p2=0 , int BCStart=0) {
		return sumIMessage(id, type, p1, p2, BCStart);
	}

	void IMBadSender(sIMessage_base * msg ,  int rcvr=0);
	int directIMessage(class cPlug & , sIMessage_base * msg);
	inline int IMessageDirect(class cPlug & plug, sIMessage_base * msg) {
		return directIMessage(plug, msg);	
	}
	inline int IMessageDirect(cPlug & plug, unsigned int  id, int  p1=0, int  p2=0 , unsigned int sender=0){
		return directIMessage(plug , &sIMessage(id,p1,p2));
	}

};