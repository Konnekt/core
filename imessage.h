
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

	int directIMessage(class cPlug & , sIMessage_base * msg);
	inline int IMessageDirect(class cPlug & plug, sIMessage_base * msg) {
		return directIMessage(plug, msg);	
	}
	inline int IMessageDirect(cPlug & plug, unsigned int  id, int  p1=0, int  p2=0 , unsigned int sender=0){
		return directIMessage(plug , &sIMessage(id,p1,p2));
	}

};