#pragma once

#include "konnekt_sdk.h"

namespace Konnekt { 

	extern int msgSent;
	extern int msgRecv;

	class MessageQueue: public LockableObject<iObject> {
	private:
		MessageQueue _inst;
	public:
		static MessageQueue& instance() {
			return _inst;
		}

		

	};





	int newMessage (cMessage * m , bool load=0 , int pos=0);
	void runMessageQueue(sMESSAGESELECT * ms, bool notifyOnly=false);
	int messageNotify(sMESSAGENOTIFY * mn);
	int messageWaiting(sMESSAGESELECT * mw);
	int getMessage(sMESSAGEPOP * mp , cMessage * m);
	int removeMessage(sMESSAGEPOP * mp , unsigned int limit);
	void messageProcessed(int id , bool remove);
	void initMessages(void);

};
