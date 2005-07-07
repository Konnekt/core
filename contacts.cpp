#include "stdafx.h"
#include "contacts.h"
#include "main.h"
#include "tables.h"
#include "konnekt_sdk.h"
#include "pseudo_export.h"

namespace Konnekt {namespace Contacts {

void updateContact(int pos) {
	if ((pos = Cnt.getrowpos(pos))==-1) return;
	SETCNTIM(pos , CNT_STATUS , IMessage(IMC_IGN_FIND , 0 , 0 , Cnt.getint(pos , CNT_NET) , (int)Cnt.getch(pos , CNT_UID))?ST_IGNORED:0 , ST_IGNORED);
}

void updateAllContacts() {
	for (unsigned int i = 0; i < Cnt.getrowcount(); i++) {
		updateContact(i);
	}
}
int findContact(int p1 , char * p2) {
	if (!p1 && !*p2) return 0;
	if (!*p2)
		return -1;
	if (p1 == 0 /*net*/) {
		int id = Ctrl->DTgetID(DTCNT, chtoint(p2));
		if (Ctrl->DTgetPos(DTCNT, id) != -1) 
			return id;
		else
			return -1;
	}
	int sz=Cnt.getrowcount();
	for (int i=1;i<sz;i++) {
		if (p1==(int)Cnt.rows[i]->get(CNT_NET)
			&&!stricmp(p2 , (char*)Cnt.rows[i]->get(p1?CNT_UID:CNT_DISPLAY)))
			return DT_MASKID(Cnt.rows[i]->id);
		//      return i;
	}
	return -1;
}

};};