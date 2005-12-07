#include "stdafx.h"
#include "contacts.h"
#include "main.h"
#include "tables.h"
#include "konnekt_sdk.h"
#include "pseudo_export.h"

using namespace Stamina;

namespace Konnekt {namespace Contacts {

void updateContact(int pos) {
	if ((pos = Tables::cnt->getRowPos(pos))==-1) return;
	int status = Tables::cnt->getInt(pos, CNT_STATUS) & ~ST_IGNORED;
	Tables::cnt->setInt(pos, CNT_STATUS, status | (IMessage(IMC_IGN_FIND , 0 , 0 , Tables::cnt->getInt(pos , CNT_NET), (int)Tables::cnt->getString(pos , CNT_UID).a_str()) ? ST_IGNORED : 0));
}

void updateAllContacts() {
	for (unsigned int i = 0; i < Tables::cnt->getRowCount(); i++) {
		updateContact(i);
	}
}
int findContact(int p1 , char * p2) {
	if (!p1 && !*p2) return 0;
	if (!*p2)
		return -1;
	if (p1 == 0 /*net*/) {
		tRowId id = Tables::cnt->getRowId(chtoint(p2));
		if (Tables::cnt->getRowPos(id) != Tables::rowNotFound) 
			return id;
		else
			return -1;
	}
	if (p1 == -1) { // tylko wg. display
		return Tables::cnt->findRow(0, DT::Find::EqStr(Tables::cnt->getColumn( CNT_DISPLAY), p2)).getId();
	} else {
		return Tables::cnt->findRow(0, DT::Find::EqInt(Tables::cnt->getColumn( CNT_NET), p1), DT::Find::EqStr(Tables::cnt->getColumn( CNT_UID), p2)).getId();
	}
	/*
	int sz = Tables::cnt->getRowCount();
	for (int i=1;i<sz;i++) {
		if (p1==(int)Cnt.rows[i]->get(CNT_NET)
			&&!stricmp(p2 , (char*)Cnt.rows[i]->get(p1?CNT_UID:CNT_DISPLAY)))
			return DT_MASKID(Cnt.rows[i]->id);
		//      return i;
	}
	*/
	return -1;
}

};};