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

		Tables::accounts = registerTable(Ctrl, tableAccounts, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Accounts");
		accounts ->setFilename("accounts.dtb");
		accounts ->setDirectory();

		Tables::cnt = registerTable(Ctrl, tableContacts, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Contacts");
		cnt->setFilename("cnt.dtb");
		cnt->setDirectory();

		oTable msg = registerTable(Ctrl, tableMessages, optBroadcastEvents | optAutoLoad | optAutoSave | optMakeBackups | optUseCurrentPassword);
		Unique::registerId(Unique::domainTable, tableConfig, "Messages");
		msg->setFilename("msg.dtb");
		msg->setDirectory();

		Tables::global = registerTable(Ctrl, tableGlobalCfg, optAutoLoad | optAutoSave | optMakeBackups | optGlobalData);
		Unique::registerId(Unique::domainTable, tableGlobalCfg, "GlobalCfg");
		global->setFilename("global.dtb");
		global->setDirectory();

		Tables::tables.addPasswordDigest(passwordDigest);
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
		msg->setColumn(Message::colId,ctypeInt , "ID");
		msg->setColumn(Message::colNet,  ctypeInt , "Net");
		msg->setColumn(Message::colType, ctypeInt , "Type");
		msg->setColumn(Message::colFromUid, ctypeString|cflagXor , "FromUID");
		msg->setColumn(Message::colToUid, ctypeString|cflagXor , "ToUID");
		msg->setColumn(Message::colBody, ctypeString|cflagXor , "Body");
		msg->setColumn(Message::colExt, ctypeString|cflagXor , "Ext");
		msg->setColumn(Message::colFlag, ctypeInt , "Flag");
		msg->setColumn(Message::colNotify, ctypeInt | cflagDontSave);
		msg->setColumn(Message::colActionP, ctypeInt | cflagDontSave);
		msg->setColumn(Message::colActionI, ctypeInt | cflagDontSave);
		msg->setColumn(Message::colHandler, ctypeInt | cflagDontSave);
		msg->setColumn(Message::colTime, ctypeInt64 , "Time");
		//msg->setColumn(Message::colAccount,  ctypeInt , "Account");

		// Konfiguracja
		cfg->setColumn(CFG_VERSIONS, ctypeString , "Versions");
		cfg->setColumn(CFG_APPFILE, ctypeString | cflagDontSave);
		cfg->setColumn(CFG_APPDIR, ctypeString | cflagDontSave);
		cfg->setColumn(CFG_AUTO_CONNECT, ctypeInt , "ConnAuto");
		cfg->setColumn(CFG_DIALUP, ctypeInt , "Dialup");
		cfg->setColumn(CFG_RETRY, ctypeInt , "ConnRetry")->setInt(rowDefault, 1);
		cfg->setColumn(CFG_LOGHISTORY , ctypeInt , "LogHistory")->setInt(rowDefault, 1);
		cfg->setColumn(CFG_TIMEOUT , ctypeInt , "Timeout")->setInt(rowDefault, 30000);
		cfg->setColumn(CFG_TIMEOUT_RETRY , ctypeInt , "TimeoutRetry")->setInt(rowDefault, 60000);
		cfg->setColumn(CFG_PROXY, ctypeInt , "Proxy/On");
		cfg->setColumn(CFG_PROXY_AUTH, ctypeInt , "Proxy/Auth");
		cfg->setColumn(CFG_PROXY_PORT, ctypeInt , "Proxy/Port")->setInt(rowDefault, 8080);
		cfg->setColumn(CFG_PROXY_AUTO, ctypeInt , "Proxy/Auto")->setInt(rowDefault, 1);
		cfg->setColumn(CFG_PROXY_HTTP_ONLY, ctypeInt , "Proxy/HttpOnly")->setInt(rowDefault, 1);
		cfg->setColumn(CFG_PROXY_HOST , ctypeString | cflagXor , "Proxy/Host");
		cfg->setColumn(CFG_PROXY_LOGIN , ctypeString | cflagXor , "Proxy/Login");
		cfg->setColumn(CFG_PROXY_PASS , ctypeString | DT_CF_SECRET | cflagXor , "Proxy/Pass");
		cfg->setColumn(CFG_PROXY_VERSION , ctypeString | cflagXor , "Proxy/Version");
		cfg->setColumn(CFG_IGNORE , ctypeString | cflagXor , "Ignore");
		cfg->setColumn(CFG_GROUPS , ctypeString | cflagXor , "Groups");
		cfg->setColumn(CFG_CURGROUP , ctypeString | cflagXor , "CurrentGroup");

		// Kontakty
    cnt->setColumn(Contact::colUid, ctypeString | cflagXor , "UID");
		cnt->setColumn(Contact::colNet, ctypeInt , "Net");
		cnt->setColumn(Contact::colStatus, ctypeInt , "Status");
		cnt->setColumn(Contact::colHost, ctypeString , "Host");
		cnt->setColumn(Contact::colPort, ctypeInt , "Port");
		cnt->setColumn(Contact::colGender, ctypeInt , "Gender")->setInt(rowDefault, Contact::genderUnknown);
		cnt->setColumn(Contact::colBorn, ctypeInt , "Born");
		cnt->setColumn(Contact::colNotify, ctypeInt | cflagDontSave);
		cnt->setColumn(Contact::colLastMsg, ctypeInt | cflagDontSave);
		cnt->setColumn(Contact::colActParent, ctypeInt|cflagDontSave);
		cnt->setColumn(Contact::colActId, ctypeInt|cflagDontSave);
		cnt->setColumn(Contact::colNotifyMsg, ctypeInt|cflagDontSave);
		cnt->setColumn(Contact::colInternal , ctypeInt|cflagDontSave);
		cnt->setColumn(Contact::colClientVersion , ctypeInt , "ClientVersion");
		cnt->setColumn(Contact::colClient , ctypeString|cflagXor , "Client");
		cnt->setColumn(Contact::colLastActivity , ctypeInt64 , "LastActivity");
		cnt->setColumn(Contact::colStatusInfo , ctypeString | cflagXor , "StatusInfo");
		cnt->setColumn(Contact::colName , ctypeString | cflagXor , "Name");
		cnt->setColumn(Contact::colSurname , ctypeString | cflagXor , "Surname");
		cnt->setColumn(Contact::colNick , ctypeString | cflagXor , "Nick");
		cnt->setColumn(Contact::colDisplay , ctypeString | cflagXor , "Display");
		cnt->setColumn(Contact::colCellPhone , ctypeString | cflagXor , "Cellphone");
		cnt->setColumn(Contact::colPhone , ctypeString | cflagXor , "Phone");
		cnt->setColumn(Contact::colMail , ctypeString | cflagXor , "EMail");
		cnt->setColumn(Contact::colInfo , ctypeString | cflagXor , "Info");
		cnt->setColumn(Contact::colLocality , ctypeString | cflagXor , "Locality");
		cnt->setColumn(Contact::colCountry , ctypeString | cflagXor , "Country");
		cnt->setColumn(Contact::colGroup , ctypeString | cflagXor , "Group");
		cnt->setColumn(Contact::colStreet , ctypeString | cflagXor , "Street");
		cnt->setColumn(Contact::colPostalCode , ctypeString | cflagXor , "PostalCode");

		cnt->setColumn(Contact::colMiddleName , ctypeString | cflagXor , "MiddleName");
		cnt->setColumn(Contact::colMailMore , ctypeString | cflagXor , "Email_More");
		cnt->setColumn(Contact::colPhoneMore , ctypeString | cflagXor , "Phone_More");
		cnt->setColumn(Contact::colDescription , ctypeString | cflagXor , "Description");
		cnt->setColumn(Contact::colFax , ctypeString | cflagXor , "Fax");
		cnt->setColumn(Contact::colUrl , ctypeString | cflagXor , "URL");
		cnt->setColumn(Contact::colAddressMore , ctypeString | cflagXor , "Address_More");
		cnt->setColumn(Contact::colRegion , ctypeString | cflagXor , "Region");
		cnt->setColumn(Contact::colPoBox , ctypeString | cflagXor , "POBox");
    cnt->setColumn(Contact::colWorkOrganization , ctypeString | cflagXor , "Work/Organization");
		cnt->setColumn(Contact::colWorkOrgUnit , ctypeString | cflagXor , "Work/OrganizationUnit");
		cnt->setColumn(Contact::colWorkTitle , ctypeString | cflagXor , "Work/Title");
		cnt->setColumn(Contact::colWorkRole , ctypeString | cflagXor , "Work/Role");
		cnt->setColumn(Contact::colWorkMail , ctypeString | cflagXor , "Work/EMail");
		cnt->setColumn(Contact::colWorkUrl , ctypeString | cflagXor , "Work/URL");
		cnt->setColumn(Contact::colWorkPhone , ctypeString | cflagXor , "Work/Phone");
		cnt->setColumn(Contact::colWorkFax , ctypeString | cflagXor , "Work/Fax");
		cnt->setColumn(Contact::colWorkStreet , ctypeString | cflagXor , "Work/Street");
		cnt->setColumn(Contact::colWorkAdressMore , ctypeString | cflagXor , "Work/Address_More");
		cnt->setColumn(Contact::colWorkPoBox , ctypeString | cflagXor , "Work/POBox");
		cnt->setColumn(Contact::colWorkPostalCode , ctypeString | cflagXor , "Work/PostalCode");
		cnt->setColumn(Contact::colWorkLocality , ctypeString | cflagXor , "Work/Locality");
		cnt->setColumn(Contact::colWorkRegion , ctypeString | cflagXor , "Work/Region");
		cnt->setColumn(Contact::colWorkCountry , ctypeString | cflagXor , "Work/Country");
		cnt->setColumn(Contact::colWorkMore , ctypeString | cflagXor , "Work/More");

		// Konta
		accounts->setColumn(Account::colHandler, ctypeInt | cflagNoSave , "Handler")->setInt(rowDefault, 0);
		accounts->setColumn(Account::colUid, ctypeString | cflagXor , "UID");
		accounts->setColumn(Account::colPassword, ctypeString | cflagXor | cflagSecret , "Password");
		accounts->setColumn(Account::colNet, ctypeInt , "Net");
		accounts->setColumn(Account::colDisplay, ctypeString | cflagXor , "Display");
		accounts->setColumn(Account::colStartStatus, ctypeInt , "StartStatus");
		accounts->setColumn(Account::colStatus, ctypeInt , "Status");
		accounts->setColumn(Account::colStatusInfo, ctypeInt , "StatusInfo");
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
			if (argVExists(ARGV_PROFILE))
				profile = getArgV(ARGV_PROFILE , true);
			else
				profile = regQueryString(hKey,"Profile");

			if (argVExists(ARGV_PROFILESDIR)) {
				profilesDir = getArgV(ARGV_PROFILESDIR , true);
			} else
				profilesDir = regQueryString(hKey,"ProfilesDir");

			profilesDir = Stamina::unifyPath(profilesDir, true);
			str = regQueryString(hKey,"Version");
			newVersion = (str != versionInfo.c_str());
			Debug::superUser=false;
	#ifdef __DEBUG
			Debug::superUser=regQueryDWord(hKey , "superUser" , 0)==1;
			if (argVExists(ARGV_DEBUG))
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
			if (!argVExists(ARGV_PROFILE)) {
				regSetString(hKey,"Profile",profile);
			}
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