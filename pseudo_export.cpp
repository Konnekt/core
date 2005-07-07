#include "stdafx.h"
#include "pseudo_export.h"
#include "imessage.h"

cCtrlEx * Konnekt::CtrlEx;

/*
void Konnekt::IMLOG(const char *format, ...) {
	va_list ap;
	static char buffer [255];
	va_start(ap, format);
	int size = _vsnprintf(buffer , sizeof(buffer) , format,ap);
	if (size==-1 || size>sizeof(buffer)) {
		char * buff = _vsaprintf(format , ap);
		IMessage(IMC_LOG , 0,0,(int)buff,0);
		free(buff);
	} else {IMessage(IMC_LOG , 0,0,(int)buffer,0);}

	va_end(ap);
}

void Konnekt::IMERROR() {  //Windows errors
	if (GetLastError()) {
		IMLOG("E[%d] \"%s\"" , GetLastError() , GetLastErrorMsg());
	}
}
*/
int Konnekt::IMessage(unsigned int id , signed int net , unsigned int type , int p1 , int p2 , int BCStart) {
	// Wysylanie wiadomosci z CORE
	return IMessageProcess(&sIMessage(id,net,type,p1,p2) , BCStart);
}

int Konnekt::UIMessage(unsigned int id , int p1 , int p2) {
	return IMessageProcess(&sIMessage(id,0,0,p1,p2) , 0);
}
/*
int Konnekt::IMessageDirect(int plugID, unsigned int  id, int  p1, int  p2 , unsigned int sender){
	int p = Plug.FindID(plugID);
	if (p<0) {TLSU().setError(IMERROR_BADPLUG);return 0;}
	return IMessageDirect(Plug[p],id,p1,p2,sender);
}
int Konnekt::IMessageDirect(int plugID, sIMessage_base * msg){
	int p = Plug.FindID(plugID);
	if (p<0) {TLSU().setError(IMERROR_BADPLUG);return 0;}
	return IMessageDirect(Plug[p],msg);
}
*/
