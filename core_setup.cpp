#include "stdafx.h"
#include "konnekt_sdk.h"
#include "tables.h"
#include "main.h"
#include "plugins.h"
#include "profiles.h"
#include "argv.h"
#include "debug.h"

using namespace Stamina;

namespace Konnekt {

	using namespace Tables;

	void initializeMainTables() {
		using namespace Tables;
		Tables::cfg = registerTable(Ctrl, tableConfig, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Config");
		cfg->setFilename("cfg.dtb");
		cfg->setDirectory();

		Tables::cnt = registerTable(Ctrl, tableContacts, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Contacts");
		cnt->setFilename("cnt.dtb");
		cnt->setDirectory();

		oTable msg = registerTable(Ctrl, tableMessages, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Messages");
		msg->setFilename("msg.dtb");
		msg->setDirectory();




	}


	void setColumns() {
		/* Msg.cols.deftype = Cfg.cols.deftype = Cnt.cols.deftype = ctypeString | DT_CT_CXOR;
		msg->setColumncount(C_MSG_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		cnt->setColumncount(C_CNT_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		cfg->setColumncount(C_CFG_COLCOUNT , DT_CC_FILL | DT_CC_RESIZE);
		*/

		using namespace Tables;

		// Ustawia w³asne 

		// Wiadomoœci
		oTableImpl msg(tableMessages);
		msg->setColumn(MSG_ID,   ctypeInt , 0 , "ID");
		msg->setColumn(MSG_NET,  ctypeInt , 0 , "Net");
		msg->setColumn(MSG_TYPE, ctypeInt , 0 , "Type");
		msg->setColumn(MSG_FROMUID, ctypeString|cflagXor , 0 , "FromUID");
		msg->setColumn(MSG_TOUID, ctypeString|cflagXor , 0 , "ToUID");
		msg->setColumn(MSG_BODY, ctypeString|cflagXor , 0 , "Body");
		msg->setColumn(MSG_EXT, ctypeString|cflagXor , 0 , "Ext");
		msg->setColumn(MSG_FLAG, ctypeInt , 0 , "Flag");
		msg->setColumn(MSG_NOTIFY, ctypeInt | cflagDontSave);
		msg->setColumn(MSG_ACTIONP, ctypeInt | cflagDontSave);
		msg->setColumn(MSG_ACTIONI, ctypeInt | cflagDontSave);
		msg->setColumn(MSG_HANDLER, ctypeInt | cflagDontSave);
		msg->setColumn(MSG_TIME, ctypeInt64 , 0 , "Time");

		// Konfiguracja
		cfg->setColumn(CFG_VERSIONS, ctypeString , "" , "Versions");
		cfg->setColumn(CFG_APPFILE, ctypeString | cflagDontSave);
		cfg->setColumn(CFG_APPDIR, ctypeString | cflagDontSave);
		cfg->setColumn(CFG_AUTO_CONNECT, ctypeInt , 0 , "ConnAuto");
		cfg->setColumn(CFG_DIALUP, ctypeInt , (void*)0 , "Dialup");
		cfg->setColumn(CFG_RETRY, ctypeInt , (void*)1 , "ConnRetry");
		cfg->setColumn(CFG_LOGHISTORY , ctypeInt , (void*)1 , "LogHistory");
		cfg->setColumn(CFG_TIMEOUT , ctypeInt , (void*)30000 , "Timeout");
		cfg->setColumn(CFG_TIMEOUT_RETRY , ctypeInt , (void*)60000 , "TimeoutRetry");
		cfg->setColumn(CFG_PROXY, ctypeInt , 0 , "Proxy/On");
		cfg->setColumn(CFG_PROXY_AUTH, ctypeInt , 0 , "Proxy/Auth");
		cfg->setColumn(CFG_PROXY_PORT, ctypeInt , (void*)8080 , "Proxy/Port");
		cfg->setColumn(CFG_PROXY_AUTO, ctypeInt , (void*)1 , "Proxy/Auto");
		cfg->setColumn(CFG_PROXY_HTTP_ONLY, ctypeInt , (void*)1 , "Proxy/HttpOnly");
		cfg->setColumn(CFG_PROXY_HOST , ctypeString | cflagXor , 0 , "Proxy/Host");
		cfg->setColumn(CFG_PROXY_LOGIN , ctypeString | cflagXor , 0 , "Proxy/Login");
		cfg->setColumn(CFG_PROXY_PASS , ctypeString | DT_CF_SECRET | cflagXor , 0 , "Proxy/Pass");
		cfg->setColumn(CFG_PROXY_VERSION , ctypeString | cflagXor , 0 , "Proxy/Version");
		cfg->setColumn(CFG_IGNORE , ctypeString | cflagXor , 0 , "Ignore");
		cfg->setColumn(CFG_GROUPS , ctypeString | cflagXor , 0 , "Groups");
		cfg->setColumn(CFG_CURGROUP , ctypeString | cflagXor , 0 , "CurrentGroup");

		// Kontakty
		cnt->setColumn(CNT_UID, ctypeString | cflagXor , 0 , "UID");
		cnt->setColumn(CNT_NET, ctypeInt , 0 , "Net");
		cnt->setColumn(CNT_STATUS, ctypeInt , 0 ,  "Status");
		cnt->setColumn(CNT_HOST, ctypeString , 0 ,  "Host");
		cnt->setColumn(CNT_PORT, ctypeInt , 0 ,  "Port");
		cnt->setColumn(CNT_GENDER, ctypeInt , GENDER_UNKNOWN ,  "Gender");
		cnt->setColumn(CNT_BORN, ctypeInt , 0 ,  "Born");
		cnt->setColumn(CNT_NOTIFY, ctypeInt | cflagDontSave);
		cnt->setColumn(CNT_LASTMSG, ctypeInt | cflagDontSave);
		cnt->setColumn(CNT_ACT_PARENT, ctypeInt|cflagDontSave);
		cnt->setColumn(CNT_ACT_ID, ctypeInt|cflagDontSave);
		cnt->setColumn(CNT_NOTIFY_MSG, ctypeInt|cflagDontSave);
		cnt->setColumn(CNT_INTERNAL , ctypeInt|cflagDontSave , 0);
		cnt->setColumn(CNT_CLIENTVERSION , ctypeInt , 0 , "ClientVersion");
		cnt->setColumn(CNT_CLIENT , ctypeString|cflagXor , 0 , "Client");
		cnt->setColumn(CNT_LASTACTIVITY , ctypeInt64 , 0 , "LastActivity");
		cnt->setColumn(CNT_STATUSINFO , ctypeString | cflagXor , 0 , "StatusInfo");
		cnt->setColumn(CNT_NAME , ctypeString | cflagXor , 0 , "Name");
		cnt->setColumn(CNT_SURNAME , ctypeString | cflagXor , 0 , "Surname");
		cnt->setColumn(CNT_NICK , ctypeString | cflagXor , 0 , "Nick");
		cnt->setColumn(CNT_DISPLAY , ctypeString | cflagXor , 0 , "Display");
		cnt->setColumn(CNT_CELLPHONE , ctypeString | cflagXor , 0 , "Cellphone");
		cnt->setColumn(CNT_PHONE , ctypeString | cflagXor , 0 , "Phone");
		cnt->setColumn(CNT_EMAIL , ctypeString | cflagXor , 0 , "EMail");
		cnt->setColumn(CNT_INFO , ctypeString | cflagXor , 0 , "Info");
		cnt->setColumn(CNT_LOCALITY , ctypeString | cflagXor , 0 , "Locality");
		cnt->setColumn(CNT_COUNTRY , ctypeString | cflagXor , 0 , "Country");
		cnt->setColumn(CNT_GROUP , ctypeString | cflagXor , 0 , "Group");
		cnt->setColumn(CNT_STREET , ctypeString | cflagXor , 0 , "Street");
		cnt->setColumn(CNT_POSTALCODE , ctypeString | cflagXor , 0 , "PostalCode");

		cnt->setColumn(CNT_MIDDLENAME , ctypeString | cflagXor , 0 , "MiddleName");
		cnt->setColumn(CNT_EMAIL_MORE , ctypeString | cflagXor , 0 , "Email_More");
		cnt->setColumn(CNT_PHONE_MORE , ctypeString | cflagXor , 0 , "Phone_More");
		cnt->setColumn(CNT_DESCRIPTION , ctypeString | cflagXor , 0 , "Description");
		cnt->setColumn(CNT_FAX , ctypeString | cflagXor , 0 , "Fax");
		cnt->setColumn(CNT_URL , ctypeString | cflagXor , 0 , "URL");
		cnt->setColumn(CNT_ADDRESS_MORE , ctypeString | cflagXor , 0 , "Address_More");
		cnt->setColumn(CNT_REGION , ctypeString | cflagXor , 0 , "Region");
		cnt->setColumn(CNT_POBOX , ctypeString | cflagXor , 0 , "POBox");
		cnt->setColumn(CNT_WORK_ORGANIZATION , ctypeString | cflagXor , 0 , "Work/Organization");
		cnt->setColumn(CNT_WORK_ORG_UNIT , ctypeString | cflagXor , 0 , "Work/OrganizationUnit");
		cnt->setColumn(CNT_WORK_TITLE , ctypeString | cflagXor , 0 , "Work/Title");
		cnt->setColumn(CNT_WORK_ROLE , ctypeString | cflagXor , 0 , "Work/Role");
		cnt->setColumn(CNT_WORK_EMAIL , ctypeString | cflagXor , 0 , "Work/EMail");
		cnt->setColumn(CNT_WORK_URL , ctypeString | cflagXor , 0 , "Work/URL");
		cnt->setColumn(CNT_WORK_PHONE , ctypeString | cflagXor , 0 , "Work/Phone");
		cnt->setColumn(CNT_WORK_FAX , ctypeString | cflagXor , 0 , "Work/Fax");
		cnt->setColumn(CNT_WORK_STREET , ctypeString | cflagXor , 0 , "Work/Street");
		cnt->setColumn(CNT_WORK_ADDRESS_MORE , ctypeString | cflagXor , 0 , "Work/Address_More");
		cnt->setColumn(CNT_WORK_POBOX , ctypeString | cflagXor , 0 , "Work/POBox");
		cnt->setColumn(CNT_WORK_POSTALCODE , ctypeString | cflagXor , 0 , "Work/PostalCode");
		cnt->setColumn(CNT_WORK_LOCALITY , ctypeString | cflagXor , 0 , "Work/Locality");
		cnt->setColumn(CNT_WORK_REGION , ctypeString | cflagXor , 0 , "Work/Region");
		cnt->setColumn(CNT_WORK_COUNTRY , ctypeString | cflagXor , 0 , "Work/Country");
		cnt->setColumn(CNT_WORK_MORE , ctypeString | cflagXor , 0 , "Work/More");


		return;
	}

	// ----------------------------------------------------------------------

	void loadRegistry() {
		HKEY hKey=HKEY_CURRENT_USER;
		string str;
		if (!RegOpenKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,KEY_READ,&hKey))
		{
			//    if (!RegOpenKeyEx(hKey,"Stamina",0,KEY_READ,&hKey) &&
			//          !RegOpenKeyEx(hKey,sig.c_str(),0,KEY_READ,&hKey))
			//       if (!RegCreateKey(hKey,"Stamina"
			//      {
			if (getArgV(ARGV_PROFILE))
				profile = getArgV(ARGV_PROFILE , true);
			else
				profile = regQueryString(hKey,"Profile");

			if (getArgV(ARGV_PROFILESDIR,true,0)) {
				profilesDir = getArgV(ARGV_PROFILESDIR , true);
			} else
				profilesDir = regQueryString(hKey,"ProfilesDir");

			profilesDir = Stamina::unifyPath(profilesDir, true);
			str=regQueryString(hKey,"Version");
			newVersion = (str != versionInfo);
			Debug::superUser=false;
	#ifdef __DEBUG
			Debug::superUser=regQueryDWord(hKey , "superUser" , 0)==1;
			if (getArgV(ARGV_DEBUG))
				Debug::superUser = true;

			if (Debug::superUser) {
				//Debug::show=RegQueryDW(hKey , "dbg_show" , 1);
				Debug::x  = regQueryDWord(hKey , "dbg_x" , 0);
				Debug::y  = regQueryDWord(hKey , "dbg_y" , 0);
				Debug::w  = regQueryDWord(hKey , "dbg_w" , 0);
				Debug::h  = regQueryDWord(hKey , "dbg_h" , 0);
				//Debug::log = RegQueryDW(hKey , "dbg_log" , 0);
				//Debug::log = false;
				//Debug::logAll = RegQueryDW(hKey , "dbg_logAll" , 0);
				//Debug::logAll = false;
				Debug::scroll = regQueryDWord(hKey , "dbg_scroll" , 0);
			}   
	#endif
			//      }
			RegCloseKey(hKey);
		}
	}


	int saveRegistry() {
		HKEY hKey=HKEY_CURRENT_USER , hKey2;
		if (
			//    !RegOpenKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,KEY_ALL_ACCESS,&hKey)
			!RegCreateKeyEx(hKey,string("Software\\Stamina\\"+sig).c_str(),0,"",0,KEY_ALL_ACCESS,0,&hKey,0)
			)
		{
			if (!getArgV(ARGV_PROFILE))
				regSetString(hKey,"Profile",profile);
			regSetString(hKey,"ProfilesDir",profilesDir);
			regSetString(hKey,"Version",versionInfo);
	#ifdef __DEBUG

			if (Debug::superUser) {
				//RegSetDW(hKey,"dbg_show",IsWindowVisible(dbg.hwnd));
				RECT rc;
				GetWindowRect(Debug::hwnd , &rc);
				regSetDWord(hKey,"dbg_x",rc.left);
				regSetDWord(hKey,"dbg_y",rc.top);
				regSetDWord(hKey,"dbg_w",rc.right - rc.left);
				regSetDWord(hKey,"dbg_h",rc.bottom - rc.top);
				//RegSetDW(hKey,"dbg_log",dbg.log);
				//RegSetDW(hKey,"dbg_logAll",dbg.logAll);
				//dbg.logAll = false;
				regSetDWord(hKey,"dbg_scroll",Debug::scroll);
			}

	#endif
			if (!RegCreateKeyEx(hKey,"DiscardPlugins",0,"",0,KEY_ALL_ACCESS,0,&hKey2,0))
			{
				RegCloseKey(hKey2);
			}
			RegCloseKey(hKey);
		}
		return 0;
	}


	// ------------------------------------------------------


	void updateCore(int from) {
		if (!from) return;
		if (from < VERSION_TO_NUM(0,6,16,85)) {
			MessageBox(0 , "Wszystkie wtyczki powinny znajdowaæ siê teraz w katalogu Plugins\\\r\n\r\nProszê o przeniesienie tam swoich wtyczek!" , "Konnekt" , MB_ICONEXCLAMATION);
		}
	}

};