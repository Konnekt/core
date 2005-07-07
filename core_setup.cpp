#include "stdafx.h"
#include "konnekt_sdk.h"
#include "tables.h"
#include "main.h"

namespace Konnekt {

	void setPlgColumns() {
		Plg.dbID = 0;
		Plg.cols.deftype = DT_CT_PCHAR | DT_CT_CXOR;
		Plg.cols.setcolcount(C_PLG_COLCOUNT , DT_CC_FILL);
		Plg.cols.setcol(PLG_LOAD,   DT_CT_INT);
		Plg.cols.setcol(PLG_NEW,   DT_CT_INT | DT_CT_DONTSAVE , (TdEntry)-1);
	}
	void setColumns() {
		/* Msg.cols.deftype = Cfg.cols.deftype = Cnt.cols.deftype = DT_CT_PCHAR | DT_CT_CXOR;
		Msg.cols.setcolcount(C_MSG_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		Cnt.cols.setcolcount(C_CNT_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		Cfg.cols.setcolcount(C_CFG_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		*/

		// Ustawia w³asne 

		// Wiadomoœci
		Msg.cols.setcol(MSG_ID,   DT_CT_INT , 0 , "ID");
		Msg.cols.setcol(MSG_NET,  DT_CT_INT , 0 , "Net");
		Msg.cols.setcol(MSG_TYPE, DT_CT_INT , 0 , "Type");
		Msg.cols.setcol(MSG_FROMUID, DT_CT_PCHAR|DT_CF_CXOR , 0 , "FromUID");
		Msg.cols.setcol(MSG_TOUID, DT_CT_PCHAR|DT_CF_CXOR , 0 , "ToUID");
		Msg.cols.setcol(MSG_BODY, DT_CT_PCHAR|DT_CF_CXOR , 0 , "Body");
		Msg.cols.setcol(MSG_EXT, DT_CT_PCHAR|DT_CF_CXOR , 0 , "Ext");
		Msg.cols.setcol(MSG_FLAG, DT_CT_INT , 0 , "Flag");
		Msg.cols.setcol(MSG_NOTIFY, DT_CT_INT | DT_CT_DONTSAVE);
		Msg.cols.setcol(MSG_ACTIONP, DT_CT_INT | DT_CT_DONTSAVE);
		Msg.cols.setcol(MSG_ACTIONI, DT_CT_INT | DT_CT_DONTSAVE);
		Msg.cols.setcol(MSG_HANDLER, DT_CT_INT | DT_CT_DONTSAVE);
		Msg.cols.setcol(MSG_TIME, DT_CT_64 , 0 , "Time");

		// Konfiguracja
		Cfg.cols.setcol(CFG_VERSIONS, DT_CT_PCHAR , "" , "Versions");
		Cfg.cols.setcol(CFG_APPFILE, DT_CT_PCHAR | DT_CT_DONTSAVE);
		Cfg.cols.setcol(CFG_APPDIR, DT_CT_PCHAR | DT_CT_DONTSAVE);
		Cfg.cols.setcol(CFG_AUTO_CONNECT, DT_CT_INT , 0 , "ConnAuto");
		Cfg.cols.setcol(CFG_DIALUP, DT_CT_INT , (void*)0 , "Dialup");
		Cfg.cols.setcol(CFG_RETRY, DT_CT_INT , (void*)1 , "ConnRetry");
		Cfg.cols.setcol(CFG_LOGHISTORY , DT_CT_INT , (void*)1 , "LogHistory");
		Cfg.cols.setcol(CFG_TIMEOUT , DT_CT_INT , (void*)30000 , "Timeout");
		Cfg.cols.setcol(CFG_TIMEOUT_RETRY , DT_CT_INT , (void*)60000 , "TimeoutRetry");
		Cfg.cols.setcol(CFG_PROXY, DT_CT_INT , 0 , "Proxy/On");
		Cfg.cols.setcol(CFG_PROXY_AUTH, DT_CT_INT , 0 , "Proxy/Auth");
		Cfg.cols.setcol(CFG_PROXY_PORT, DT_CT_INT , (void*)8080 , "Proxy/Port");
		Cfg.cols.setcol(CFG_PROXY_AUTO, DT_CT_INT , (void*)1 , "Proxy/Auto");
		Cfg.cols.setcol(CFG_PROXY_HTTP_ONLY, DT_CT_INT , (void*)1 , "Proxy/HttpOnly");
		Cfg.cols.setcol(CFG_PROXY_HOST , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Proxy/Host");
		Cfg.cols.setcol(CFG_PROXY_LOGIN , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Proxy/Login");
		Cfg.cols.setcol(CFG_PROXY_PASS , DT_CT_PCHAR | DT_CF_SECRET | DT_CF_CXOR , 0 , "Proxy/Pass");
		Cfg.cols.setcol(CFG_PROXY_VERSION , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Proxy/Version");
		Cfg.cols.setcol(CFG_IGNORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Ignore");
		Cfg.cols.setcol(CFG_GROUPS , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Groups");
		Cfg.cols.setcol(CFG_CURGROUP , DT_CT_PCHAR | DT_CF_CXOR , 0 , "CurrentGroup");

		// Kontakty
		Cnt.cols.setcol(CNT_UID, DT_CT_PCHAR | DT_CF_CXOR , 0 , "UID");
		Cnt.cols.setcol(CNT_NET, DT_CT_INT , 0 , "Net");
		Cnt.cols.setcol(CNT_STATUS, DT_CT_INT , 0 ,  "Status");
		Cnt.cols.setcol(CNT_HOST, DT_CT_PCHAR , 0 ,  "Host");
		Cnt.cols.setcol(CNT_PORT, DT_CT_INT , 0 ,  "Port");
		Cnt.cols.setcol(CNT_GENDER, DT_CT_INT , GENDER_UNKNOWN ,  "Gender");
		Cnt.cols.setcol(CNT_BORN, DT_CT_INT , 0 ,  "Born");
		Cnt.cols.setcol(CNT_NOTIFY, DT_CT_INT | DT_CF_NOSAVE);
		Cnt.cols.setcol(CNT_LASTMSG, DT_CT_INT | DT_CF_NOSAVE);
		Cnt.cols.setcol(CNT_ACT_PARENT, DT_CT_INT|DT_CF_NOSAVE);
		Cnt.cols.setcol(CNT_ACT_ID, DT_CT_INT|DT_CF_NOSAVE);
		Cnt.cols.setcol(CNT_NOTIFY_MSG, DT_CT_INT|DT_CF_NOSAVE);
		Cnt.cols.setcol(CNT_INTERNAL , DT_CT_INT|DT_CF_NOSAVE , 0);
		Cnt.cols.setcol(CNT_CLIENTVERSION , DT_CT_INT , 0 , "ClientVersion");
		Cnt.cols.setcol(CNT_CLIENT , DT_CT_PCHAR|DT_CF_CXOR , 0 , "Client");
		Cnt.cols.setcol(CNT_LASTACTIVITY , DT_CT_64 , 0 , "LastActivity");
		Cnt.cols.setcol(CNT_STATUSINFO , DT_CT_PCHAR | DT_CF_CXOR , 0 , "StatusInfo");
		Cnt.cols.setcol(CNT_NAME , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Name");
		Cnt.cols.setcol(CNT_SURNAME , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Surname");
		Cnt.cols.setcol(CNT_NICK , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Nick");
		Cnt.cols.setcol(CNT_DISPLAY , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Display");
		Cnt.cols.setcol(CNT_CELLPHONE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Cellphone");
		Cnt.cols.setcol(CNT_PHONE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Phone");
		Cnt.cols.setcol(CNT_EMAIL , DT_CT_PCHAR | DT_CF_CXOR , 0 , "EMail");
		Cnt.cols.setcol(CNT_INFO , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Info");
		Cnt.cols.setcol(CNT_LOCALITY , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Locality");
		Cnt.cols.setcol(CNT_COUNTRY , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Country");
		Cnt.cols.setcol(CNT_GROUP , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Group");
		Cnt.cols.setcol(CNT_STREET , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Street");
		Cnt.cols.setcol(CNT_POSTALCODE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "PostalCode");

		Cnt.cols.setcol(CNT_MIDDLENAME , DT_CT_PCHAR | DT_CF_CXOR , 0 , "MiddleName");
		Cnt.cols.setcol(CNT_EMAIL_MORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Email_More");
		Cnt.cols.setcol(CNT_PHONE_MORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Phone_More");
		Cnt.cols.setcol(CNT_DESCRIPTION , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Description");
		Cnt.cols.setcol(CNT_FAX , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Fax");
		Cnt.cols.setcol(CNT_URL , DT_CT_PCHAR | DT_CF_CXOR , 0 , "URL");
		Cnt.cols.setcol(CNT_ADDRESS_MORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Address_More");
		Cnt.cols.setcol(CNT_REGION , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Region");
		Cnt.cols.setcol(CNT_POBOX , DT_CT_PCHAR | DT_CF_CXOR , 0 , "POBox");
		Cnt.cols.setcol(CNT_WORK_ORGANIZATION , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Organization");
		Cnt.cols.setcol(CNT_WORK_ORG_UNIT , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/OrganizationUnit");
		Cnt.cols.setcol(CNT_WORK_TITLE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Title");
		Cnt.cols.setcol(CNT_WORK_ROLE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Role");
		Cnt.cols.setcol(CNT_WORK_EMAIL , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/EMail");
		Cnt.cols.setcol(CNT_WORK_URL , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/URL");
		Cnt.cols.setcol(CNT_WORK_PHONE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Phone");
		Cnt.cols.setcol(CNT_WORK_FAX , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Fax");
		Cnt.cols.setcol(CNT_WORK_STREET , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Street");
		Cnt.cols.setcol(CNT_WORK_ADDRESS_MORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Address_More");
		Cnt.cols.setcol(CNT_WORK_POBOX , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/POBox");
		Cnt.cols.setcol(CNT_WORK_POSTALCODE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/PostalCode");
		Cnt.cols.setcol(CNT_WORK_LOCALITY , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Locality");
		Cnt.cols.setcol(CNT_WORK_REGION , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Region");
		Cnt.cols.setcol(CNT_WORK_COUNTRY , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/Country");
		Cnt.cols.setcol(CNT_WORK_MORE , DT_CT_PCHAR | DT_CF_CXOR , 0 , "Work/More");


		return;
	}


	void updateCore(int from) {
		if (!from) return;
		if (from < VERSION_TO_NUM(0,6,16,85)) {
			MessageBox(0 , "Wszystkie wtyczki powinny znajdowaæ siê teraz w katalogu Plugins\\\r\n\r\nProszê o przeniesienie tam swoich wtyczek!" , "Konnekt" , MB_ICONEXCLAMATION);
		}
	}

};