#pragma once

#include "konnekt_sdk.h"

/*
#define IMESSAGE_TS() \
	if (Ctrl->Is_TUS(0)) return Ctrl->RecallTS()
#define IMESSAGE_TS_NOWAIT(ret) \
	if (Ctrl->Is_TUS(0)) {Ctrl->RecallTS(0,0);return ret;}

#define SETCNTI(row , id , val) (int)(Ctrl->DTsetInt(DTCNT , (row) , (id) , (int)(val)))
	*/
#define SETCNTIM(row , id , val , mask) (int)(Ctrl->DTsetInt(DTCNT , (row) , (id) , (int)(val) , (mask)))

#undef ISRUNNING
#define ISRUNNING() if (!isRunning) {TLSU().setError(IMERROR_SHUTDOWN); return 0;}


namespace Konnekt {
	extern cCtrlEx * CtrlEx;

/*	void IMERROR();
	void IMLOG(const char *format, ...);
	void IMDEBUG(const char *format, ...);
*/	
	int UIMessage(unsigned int id , int p1=0 , int p2=0);
	int IMessage(unsigned int id , signed int net , unsigned int type , int p1=0 , int p2=0 , int=1);

};