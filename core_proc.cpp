/*

Obs³uga "rdzeniowych" komunikatów

*/

#include "stdafx.h"

#include <Stamina\Debug.h>

#include "konnekt_sdk.h"
#include "main.h"
#include "plugins.h"
#include "imessage.h"
#include "threads.h"
#include "profiles.h"
#include "messages.h"
#include "beta.h"
#include "tables.h"
#include "contacts.h"
#include "pseudo_export.h"
#include "resources.h"
#include "debug.h"
#include "connections.h"
#include "mru.h"
#include "unique.h"
#include "test.h"
#include "argv.h"
#include "plugins_ctrl.h"

#include <Konnekt/plugin_test.h>

using namespace Stamina;


//  IMessage do Core
int __stdcall Konnekt::coreIMessageProc(sIMessage_base * msgBase) {
	sIMessage_2params * msg = static_cast<sIMessage_2params*>(msgBase);
	if (msgBase->id == IMC_LOG) {
		return 0;
	}  
	switch (msg->id) {


		case IM_PLUG_NET:        return NET_NONE;
		case IM_PLUG_TYPE:       return IMT_ALL & ~(IMT_MSGUI | IMT_NETSEARCH | IMT_NETUID);
		case IM_PLUG_VERSION:    return (int)"";
		case IM_PLUG_SDKVERSION: return KONNEKT_SDK_V;
		case IM_PLUG_SIG:        return (int)"CORE";
		case IM_PLUG_CORE_V:     return (int)"W98";
		case IM_PLUG_UI_V:       return 0;
		case IM_PLUG_NAME:       return (int)"Core";
		case IM_PLUG_NETNAME:    return (int)"";
		case IM_PLUG_INIT:       return Plug_Init(msg->p1,msg->p2);
		case IM_PLUG_DEINIT:     return 1;

		case IMC_ISNEWVERSION: 
			return newVersion;

		case IMC_ISNEWPROFILE: 
			return newProfile;

		case IMC_ISWINXP: 
			return isComctl(6,0);

		case IMC_THREADSTART: 
			Ctrl->onThreadStart();
			return 1;//IMessage(IM_THREADSTART , NET_BC , -1 , 0 , 0 , 0);

		case IMC_THREADEND: 
			Ctrl->onThreadEnd();
			return 1;//IMessage(IM_THREADEND , NET_BC , -1 , 0 , 0 , 0);

		case IMC_RESTORECURDIR: 
			_chdir(appPath.c_str()); 
			return (int)appPath.c_str();

		case IMC_GETBETALOGIN: 
#ifdef __BETA
			return (int)Beta::betaLogin.c_str();
#else
			return (int)"";
#endif

		case IMC_GETBETAPASSMD5: 
#ifdef __BETA
			return (int)Beta::betaPass.c_str();
#else
			return (int)"";
#endif

		case IMC_GETBETAANONYMOUS: 
#ifdef __BETA
			return Beta::anonymous;
#else
			return true;
#endif

		case IMC_GETSERIALNUMBER: {
			string serial = Beta::info_serial();
			char * buff = (char*)Ctrl->GetTempBuffer(serial.length() + 1);
			strcpy(buff, serial.c_str());
			return (int) buff;}
			
		case IMC_SHUTDOWN: {
			ISRUNNING();
			IMESSAGE_TS();
			IMLOG("- IMC_SHUTDOWN (od %s) ... " , plugins.getName(msg->sender).c_str());
			if (!msg->p1 && (!canQuit || IMessage(IM_CANTQUIT , Net::Broadcast().setResult(Net::Broadcast::resultSum), IMT_ALL, 0, 0))) {
				IMLOG("- IMC_SHUTDOWN anulowany");
				return 0;
			}
			running = false;
			if (msg->p1) {
				canWait = false; // natychmiastowy shutDown...
			}
			deinitialize(true);
			return 1;}

		case IMC_NET_TYPE: {
			int type = 0;
			for (int i = 0; i < plugins.count(); i++) {
				if (plugins[i].getNet() == msg->p1) type |= plugins[i].getType();
			}
			return type;}

		case IMC_DISCONNECT: 
			ISRUNNING();
			IMessage(IM_DISCONNECT , Net::broadcast , IMT_PROTOCOL); 
			return 0;

		case IMC_NEWMESSAGE: 
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::newMessage((cMessage *)msg->p1);

		case IMC_MESSAGEACK: 
			ISRUNNING(); 
			if (((cMessageAck*)msg->p1)->flag & MACK_NOBROADCAST) {
				plugins[pluginUI].IMessageDirect(IM_MSG_ACK , msg->p1 , 0); 
			} else {
				IMessage(IM_MSG_ACK, Net::broadcast, imtMessageAck, msg->p1, 0); 
			}
			return true;

		case IMC_MESSAGEQUEUE:
			ISRUNNING();
			IMESSAGE_TS();
			Messages::runMessageQueue((sMESSAGESELECT*)msg->p1 , msg->p2);
			return 0;

		case IMC_MESSAGENOTIFY: 
			return Messages::messageNotify((sMESSAGENOTIFY *)msg->p1);

		case IMC_MESSAGEWAITING: 
			return Messages::messageWaiting((sMESSAGEWAITING *)msg->p1);

		case IMC_MESSAGEREMOVE:
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::removeMessage((sMESSAGEWAITING *)msg->p1,msg->p2);

		case IMC_MESSAGEGET: 
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::getMessage((sMESSAGEWAITING *)msg->p1 , (cMessage*)msg->p2);

		case IMC_MESSAGEPROCESSED: 
			ISRUNNING();
			IMESSAGE_TS();
			Messages::messageProcessed(msg->p1 , msg->p2); return 1;

		case IMC_FINDCONTACT: 
			return Contacts::findContact(msg->p1,(char *)msg->p2);

		case IMC_CNT_CHANGED: {
			ISRUNNING();
			IMESSAGE_TS();
			sIMessage_CntChanged msgCC(msg);
			Contacts::updateContact(msgCC._cntID);
			if (Tables::cnt->getInt(msgCC._cntID,CNT_INTERNAL)&1) {
				Tables::cnt->setInt(msgCC._cntID,CNT_INTERNAL, Tables::cnt->getInt(msgCC._cntID,CNT_INTERNAL)& (~1));
				IMessage(IM_CNT_ADD,NET_BC,IMT_CONTACT, Tables::cnt->getRowId(msgCC._cntID),0);
			}
			else {
				msgCC.id = IM_CNT_CHANGED;
				msgCC.net = NET_BC;
				msgCC.type = IMT_CONTACT;
				// Trzeba upewniæ siê, ¿e oba pola s¹ wype³nione
				if (msgCC._changed.net || msgCC._changed.uid) {
					if (!msgCC._oldUID) {
						String& temp = TLSU().buffer().getString(true);
						temp = Tables::cnt->getString(msgCC._cntID , CNT_UID);
						msgCC._oldUID = temp.c_str();
					}
					if (!msgCC._changed.net && msgCC._oldNet == NET_NONE) msgCC._oldNet = Tables::cnt->getInt(msgCC._cntID , CNT_NET);
				}
				IMessage(&msgCC);
			}
			break;}

		case IMC_CNT_IDEXISTS: 
			return Tables::cnt->getRowPos(msg->p1) != Tables::rowNotFound;         

		case IMC_CNT_ADD: {
			ISRUNNING();
			IMESSAGE_TS();
			int cnt = -1;
			if (msg->p1 && msg->p2 && *(char*)msg->p2) {
				int cnt = Contacts::findContact(msg->p1,(char *)msg->p2);
				if (cnt != -1) {
					int r = IMessage(&sIMessage_msgBox(IMI_CONFIRM, stringf(loadString(IDS_ASK_CNTOVERWRITE).c_str(), msg->p2, msg->p1, Tables::cnt->getString(cnt, CNT_DISPLAY)).c_str(), "Dodawanie kontaktu", MB_YESNOCANCEL));
					switch (r) {
						case IDNO:cnt=-1;break;
						case IDCANCEL:return -1;
					}
				}
			}
			if (cnt == -1) {
				cnt = Tables::cnt->getRowId(Tables::cnt->addRow());
				//if (!msg->p1) msg->p2 = (int)inttostr(Tables::cnt->getInt(a , DT_C_ID));
				if (msg->p1) Tables::cnt->setInt(cnt , CNT_NET , msg->p1);
				if (msg->p2) Tables::cnt->setString(cnt , CNT_UID , (char*)msg->p2);
				Tables::cnt->setInt(cnt , CNT_INTERNAL , Tables::cnt->getInt(cnt , CNT_INTERNAL)|1);
				IMessage(IM_CNT_ADDING, Net::broadcast, imtContact, cnt, 0);
			}
			return cnt;}

		case IMC_CNT_REMOVE: 
			ISRUNNING(); 
			if (msg->p1>0 && (!msg->p2
								 || ICMessage(IMI_CONFIRM
								 ,(int)stringf(loadString(IDS_ASK_CNTREMOVE).c_str(),Tables::cnt->getString(msg->p1,CNT_DISPLAY).c_str()).c_str()
								 ,0))) 
			{
				Messages::removeMessage(&sMESSAGESELECT(Tables::cnt->getInt(msg->p1,CNT_NET),Tables::cnt->getString(msg->p1,CNT_UID).c_str(),MT_MESSAGE,0,MF_SEND),-1);
				IMessage(IM_CNT_REMOVE, NET_BC, IMT_CONTACT, msg->p1, msg->p2);
				bool result = Tables::cnt->removeRow(msg->p1);
				IMessage(IM_CNT_REMOVED, NET_BC, IMT_CONTACT, msg->p1, msg->p2);
				return result;
			} else {
				return 0;
			}

		case IMC_CNT_COUNT: 
			return Tables::cnt->getRowCount();

		case IMC_CFG_SETCOL: 
		case IMC_CNT_SETCOL: {
			sSETCOL * setcol = reinterpret_cast<sSETCOL*>(msg->p1);
			sIMessage_setColumn sc(msgBase->id == IMC_CFG_SETCOL?DTCFG:DTCNT , setcol->id , setcol->type , setcol->def , setcol->name);
			sc.sender = msgBase->sender;
			return Tables::setColumn(&sc);
			}

		case sIMessage_setColumn::__msgID:
			return Tables::setColumn(static_cast<sIMessage_setColumn*>(msgBase));

		case IMC_CFG_SAVE: 
			ISRUNNING();
			//IMESSAGE_TS_NOWAIT(1);
			/*IMessage(IM_CFG_SAVE,-1,IMT_ALL,0,0,0);*/ /*saveProfile();*/
			Tables::saveProfile(false);
			return 1;

		case IMC_GETINSTANCE: return (int)Stamina::getHInstance();

		case IMC_GETPROFILE: return (int)profile.c_str();

		case IMC_LOGDIR: return (int)Debug::logPath.c_str();

		case IMC_TEMPDIR: return (int)tempPath.c_str();

		case IMC_PLUG_COUNT: return plugins.count();

		case IMC_PLUG_HANDLE: return (int)plugins[msg->p1].getDllModule();

		case IMC_PLUG_ID: return (int)plugins[msg->p1].getId();

		case IMC_PLUG_FILE:  {
			String& temp = TLSU().buffer().getString(true);
			temp = plugins[msg->p1].getDllFile().c_str();
			return (int)temp.c_str();
		}

		case IMC_PLUGID_POS: return plugins.getIndex((tPluginId)msg->p1);

		case IMC_PLUGID_HANDLE: {
			Plugin* plugin = plugins.get((tPluginId)msg->p1);
			return plugin ? (int) plugin->getDllModule() :0;
			}

		case IMC_FINDPLUG: {
			int pos = plugins.findPlugin((tNet)msg->p1, (enIMessageType)msg->p2 , 0);
			if (pos == -1) return 0;
			return plugins[pos].getId();
			}

		case IMC_FINDPLUG_BYNAME: {
			if (!msg->p1) {
				return 0;
			}
			Plugin* plugin = plugins.findName((char*) msg->p1);
			return plugin ? plugin->getId() : 0;
		}

		case IMC_FINDPLUG_BYSIG: {
			if (!msg->p1) {
				return 0;
			}
			Plugin* plugin = plugins.findSig((char*) msg->p1);
			return plugin ? plugin->getId() : 0;
		}

		case sIMessage_plugOut::__msgID: {
			ISRUNNING();
			IMESSAGE_TS();
			sIMessage_plugOut * po = static_cast<sIMessage_plugOut*>(msgBase);
			tPluginId ID = plugins.getId((tPluginId)po->_plugID);
			tPluginId senderID = plugins.getId(po->sender);
			if (ID == pluginNotFound || senderID == pluginNotFound) return 0;

			CStdString reason = po->_reason;
			CStdString namePlug = plugins[ID].getName().a_str();
			CStdString nameSender = plugins[senderID].getName().a_str();

			plugins[ID].plugOut(plugins[senderID].getCtrl(), reason, true, (iPlugin::enPlugOutUnload)po->_unload);

			CStdString msg = loadString(IDS_INF_PLUGOUT);
			//Wtyczka %s od³¹czy³a wtyczkê %s\r\nMo¿esz j¹ przywróciæ rêcznie w opcji "wtyczki".\r\n\r\n%s\r\n\r\n%s
			CStdString restart = 
				po->_restart == sIMessage_plugOut::erAsk ? loadString(IDS_ASK_RESTART) 
				: po->_restart == sIMessage_plugOut::erYes ? loadString(IDS_INF_RESTART) 
				: po->_restart == sIMessage_plugOut::erAskShut ? loadString(IDS_ASK_SHUTDOWN) 
				: po->_restart == sIMessage_plugOut::erShut ? loadString(IDS_INF_SHUTDOWN) 
				: "";
			msg.Format(msg , nameSender , namePlug , reason , restart.c_str());
			IMLOG("plugOut\n%s" , msg.c_str());
			if (MessageBox(NULL , msg , loadString(IDS_APPNAME).c_str() , MB_ICONWARNING|MB_TASKMODAL|(po->_restart & sIMessage_plugOut::erAsk ? MB_YESNO : MB_OK)) == IDNO) 
				return 1;

			if (po->_restart >= sIMessage_plugOut::erAskShut)
				ICMessage(IMC_SHUTDOWN, 1);
			else if (po->_restart >= sIMessage_plugOut::erAsk)
				ICMessage(IMC_RESTART);
			return 1;}

		case IMC_STATUSCHANGE: {
			ISRUNNING();
			IMESSAGE_TS();
			sIMessage_StatusChange sc(msgBase);
			sc.net = NET_BC;
			sc.type = IMT_ALL;
			sc.id = IM_STATUSCHANGE;
			sc.plugID = sc.sender;
			sc.sender = pluginCore;
			return IMessage(&sc);
		}

		case IMC_CNT_SETSTATUS: {
			ISRUNNING();       
			IMESSAGE_TS();
			sIMessage_StatusChange sc(msgBase);
			sc.net = NET_BC;
			sc.type = IMT_ALL;
			sc.id = IM_CNT_STATUSCHANGE;
			sc.sender = pluginCore;
			IMessage(&sc);
			if (sc.status != -1) Tables::cnt->setInt(sc.cntID , CNT_STATUS , (Tables::cnt->getInt(sc.cntID , CNT_STATUS) & ~ST_MASK) | sc.status);
			if (sc.info) Tables::cnt->setString(sc.cntID , CNT_STATUSINFO , sc.info);
			return 0;
			}

		case IMC_ARGC: return __argc;

		case IMC_ARGV: return (int) __argv[msg->p1];

		case IMC_ISDEBUG: return Debug::superUser;

		case IMC_ISBETA: return
#ifdef __BETA
							 1
#else
							 0
#endif
							 ;

		case IMC_DEBUG:
			ISRUNNING();
#ifdef __DEBUG
			if (Debug::superUser) ShowWindow(Debug::hwnd , msg->p1?SW_SHOW:SW_HIDE);
			if (msg->p1) SetForegroundWindow(Debug::hwnd);
#endif
			break;

		case IMC_BETA:
			ISRUNNING();
#ifdef __BETA
			IMESSAGE_TS();
			Beta::showBeta();
#endif
			break;

		case IMC_REPORT:
			ISRUNNING();
#ifdef __BETA
			IMESSAGE_TS();
			Beta::showReport();
#endif
			break;

		case IMC_PLUGS:
			ISRUNNING();
			IMESSAGE_TS();
			Plugins::setPlugins(true , false);
			PlugsDialog(); 
			return 1;

		case IMC_CONNECTED:
			return Connections::isConnected();

		case IMC_VERSION: 
		{
			FileVersion fv(appFile);
			if (msg->p1) {
				strcpy((char*)msg->p1, fv.getFileVersion().getString().c_str());
			}
			return fv.getFileVersion().getInt();
		}

		case IMC_PLUG_VERSION:
		{
			if (msg->p1 == -1) {
				return ICMessage(IMC_VERSION , msg->p2);
			}
			if (!plugins.exists((tPluginId)msg->p1)) return 0;
			if (msg->p2) {strcpy((char*)msg->p2 , plugins[msg->p1].getVersion().getString().c_str());}
			return plugins[msg->p1].getVersion().getInt();
		}

		case IMC_CNT_IGNORED: 
		{
			std::string list = Tables::cfg->getString(0,CFG_IGNORE);
			std::string item = stringf("\n%d@%s\n",msg->p1,msg->p2);
			return find_noCase(list.c_str(), item.c_str()) != std::string::npos;
		}

		case IMC_CNT_INGROUP: 
		{
			std::string group;
			if (msg->p2) {
				group = (char*)msg->p2;
			} else {
				group = Tables::cfg->getString(0 , CFG_CURGROUP);
			}
			if (group.empty()) { // sprawdzamy grupê pust¹
				if (Tables::cfg->getInt(0, CFG_UIALLGROUPS_NOGROUP)) {
					return Tables::cnt->getString(msg->p1 , CNT_GROUP).empty(); // wpuszczamy tylko puste...
				} else
					return 1; // wszystkie tu pasuj¹
			}
			return _stricmp(Tables::cnt->getString(msg->p1 , CNT_GROUP).c_str() , group.c_str()) == 0;
		}

		case IMC_IGN_ADD:
			ISRUNNING();
			IMESSAGE_TS();
			if (!ICMessage(IMC_IGN_FIND , msg->p1 , msg->p2)) {
				std::string list = Tables::cfg->getString(0,CFG_IGNORE);
				if (list.empty()) list = "\n";
				list += inttostr(msg->p1) + "@";
				list += (char*) msg->p2;
				list += "\n";

				Contacts::updateContact(Contacts::findContact(msg->p1 , (char*)msg->p2));
				IMessage(IM_IGN_CHANGED , NET_BC , IMT_CONTACT , msg->p1 , msg->p2);
				return 1;
			}
			return 0;

		case IMC_IGN_DEL: 
		{
			ISRUNNING();
			IMESSAGE_TS();
			std::string item = stringf("\n%d@%s\n", msg->p1 , msg->p2);
			std::string list = Tables::cfg->getString(0,CFG_IGNORE);
			size_t found = find_noCase(list.c_str(), item.c_str());
			if (found != std::string::npos) {
				list.erase(found, item.length() - 1);
				Tables::cfg->setString(0, CFG_IGNORE, list);
				Contacts::updateContact(Contacts::findContact(msg->p1 , (char*)msg->p2));
				IMessage(IM_IGN_CHANGED , NET_BC , IMT_CONTACT , -msg->p1 , msg->p2);
			}
			return 0;
		}

		case IMC_GRP_FIND: 
		{
			return find_noCase(Tables::cfg->getString(0,CFG_GROUPS).c_str() , stringf("\n%s\n",msg->p1).c_str()) != std::string::npos;
		}

		case IMC_GRP_ADD: 
		{
			ISRUNNING();
			IMESSAGE_TS();
			if (!ICMessage(IMC_GRP_FIND , msg->p1)) {
				std::string list = Tables::cfg->getString(0, CFG_GROUPS);
				if (list.empty()) list = "\n";
				list += (char*)msg->p1;
				list += "\n";
				Tables::cfg->setString(0, CFG_GROUPS, list);
				IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0);
				return 1;
			}
			return 0;
		}

		case IMC_GRP_DEL: 
		{
			ISRUNNING();
			IMESSAGE_TS();
			std::string item = (char*)msg->p1;
			item = "\n" + item + "\n";
			std::string list = Tables::cfg->getString(0, CFG_GROUPS);
			size_t found = find_noCase(list.c_str(), item.c_str());
			if (found != std::string::npos) {
				list.erase(found, item.length() - 1);
				Tables::cfg->setString(0, CFG_GROUPS, list);
				for (unsigned int i = 1; i < (int)Tables::cnt->getRowCount() ; i++) {
					if (Tables::cnt->getString(i,CNT_GROUP).equal((char*)msg->p1, true)) {
						Tables::cnt->setString(i, CNT_GROUP, "");
						sIMessage_CntChanged cc(IM_CNT_CHANGED , i);
						cc._changed.group = true;
						Ctrl->IMessage(&cc);
					}
				}
				IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0);
			}
			return 0;
		}

		case IMC_GRP_RENAME:
		{
			ISRUNNING();
			IMESSAGE_TS();
			if (!ICMessage(IMC_GRP_FIND , msg->p1) || ICMessage(IMC_GRP_FIND , msg->p2)) return 0;
			//            IMLOG("GRP_RENAME %s %s" , msg->p1 , msg->p2);
			// Usuwamy
			Tables::cfg->setString(0, CFG_GROUPS, RegEx::doReplace(("/^" + std::string((char*)msg->p1) + "$/mi").c_str(), (const char*)msg->p2, Tables::cfg->getString(0, CFG_GROUPS).c_str(), 1) ); 

			for (int i = 1; i < (int)Tables::cnt->getRowCount() ; i++) {
				if (_stricmp((char*)msg->p1 , Tables::cnt->getString(i, CNT_GROUP).c_str()) == 0) {
					Tables::cnt->setString(i , CNT_GROUP , (char*)msg->p2);
					sIMessage_CntChanged cc(IM_CNT_CHANGED , i);
					cc._changed.group = true;
					Ctrl->IMessage(&cc);
				}
			}
			IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0);
			return 0;
		}

		case IMC_PROFILEDIR:
			return (int)profileDir.c_str();

		case IMC_PROFILESDIR:
			return (int)profilesDir.c_str();

		case IMC_RESTART:
			ISRUNNING();
			IMESSAGE_TS();
			deinitialize(false);
			restart();
			return 1;

		case IMC_PROFILECHANGE:
		{
			ISRUNNING();
			IMESSAGE_TS();
			if (_access((profilesDir+(char*)msg->p1).c_str() , 0)) {
				if (!msg->p2) return 0;
				else
					_mkdir((profilesDir+(char*)msg->p1).c_str());
			}
			profile = (char*)msg->p1;
			deinitialize(false);
			restart(false);
			//            Init();
			//            IMCore(IMC_SHUTDOWN , 0 , 0 , 0);
			return 1;
		}

		case IMC_PROFILEREMOVE:
			ISRUNNING();
			IMESSAGE_TS();
			profile = "?";
			//            atexit_remove();
			deinitialize(false);
			removeProfileAndRestart();
			//            IMCore(IMC_SHUTDOWN , 0 , 0 , 0);
			return 0;

		case IMC_PROFILEPASS:
			ISRUNNING();
			profilePass();
			return 0;

		case IMC_CFG_CHANGED:
			ISRUNNING();
			IMESSAGE_TS();
			IMessage(IM_CFG_CHANGED , NET_BC , IMT_CONFIG , 0 , 0);
			return 0;

		case IMC_SAVE_CFG:
			ISRUNNING();
			IMESSAGE_TS();
			tables[Tables::tableConfig]->save();
			return 1;

		case IMC_SAVE_CNT:
			ISRUNNING();
			IMESSAGE_TS();
			tables[Tables::tableContacts]->save();
			return 1;

		case IMC_SAVE_MSG:
			ISRUNNING();
			IMESSAGE_TS();
			tables[Tables::tableMessages]->save();
			return 1;

		case IMC_SETCONNECT:
			ISRUNNING();
			IMESSAGE_TS_NOWAIT(0); /// <---- tu by³ NOWAIT!
			Connections::setConnect(msg->sender, msg->p1);
			return 0;

		case IMC_GETMSGCOLDESC:
			return (int)&tables[Tables::tableMessages]->getDT().getColumns();

		case IMC_GETMD5DIGEST:
			if (!msg->p1 || !passwordDigest.empty()) return 0;
			memcpy(&msg->p1 , passwordDigest.getDigest() , 16);
			return 1;

		case IMC_MRU_GET: 
			if (msgBase->s_size < sizeof(sIMessage_MRU)) return 0;
			return MRU::get(static_cast<sIMessage_MRU*>(msgBase)->MRU);

		case IMC_MRU_SET: 
			if (msgBase->s_size < sizeof(sIMessage_MRU)) return 0;
			return MRU::set(static_cast<sIMessage_MRU*>(msgBase)->MRU);

		case IMC_MRU_UPDATE: 
			if (msgBase->s_size < sizeof(sIMessage_MRU)) return 0;
			return MRU::update(static_cast<sIMessage_MRU*>(msgBase)->MRU);

		case IMC_SESSION_ID:
			return (int) sessionName.c_str();

		case IMC_GET_HINTERNET:
			return (int)hInternet;

		case IMC_HINTERNET_OPEN: 
		{
			HINTERNET h;
			std::string proxy;
			if (Tables::cfg->getInt(0,CFG_PROXY)) {
				proxy = Tables::cfg->getString(0,CFG_PROXY_HOST) + ":";
				if (Tables::cfg->getInt(0,CFG_PROXY_PORT)) {
					proxy += inttostr(Tables::cfg->getInt(0,CFG_PROXY_PORT));
				} else {
					proxy += "8080";
				}
			}
			h = InternetOpen(msg->p1 ? (char*)msg->p1 : "Konnekt", proxy.empty() ? INTERNET_OPEN_TYPE_DIRECT : INTERNET_OPEN_TYPE_PROXY , proxy.empty()? 0 : proxy.c_str(), 0, 0);
			if (!proxy.empty() && Tables::cfg->getInt(0,CFG_PROXY_AUTH)) {
				std::string login = Tables::cfg->getString(0,CFG_PROXY_LOGIN);
				InternetSetOption(h, INTERNET_OPTION_PROXY_USERNAME,
					(void*)login.c_str(), login.length() + 1);
				std::string pass = Tables::cfg->getString(0,CFG_PROXY_PASS);
				InternetSetOption(h, INTERNET_OPTION_PROXY_PASSWORD,
					(void*)pass.c_str(), pass.length() + 1);
			}

			return (int)h;
		}

		case IMC_GET_MAINTHREAD:
			return (int) hMainThread;

		case IMC_DEBUG_COMMAND: 
		{
			sIMessage_debugCommand * msgDc = static_cast<sIMessage_debugCommand*> (msgBase);
			if (msgDc->async == sIMessage_debugCommand::asynchronous) {
				const char ** argv = new const char * [msgDc->argc];
				for (int i = 0; i < msgDc->argc; i++)
					argv[i] = _strdup(msgDc->argv[i]);
				sIMessage_debugCommand dc (msgDc->argc, argv, sIMessage_debugCommand::duringAsynchronous);
				return Ctrl->RecallIMTS(0, false, &dc, pluginCore);
			}
			IMESSAGE_TS();
			sIMessage_debugCommand dc = *msgDc;
			dc.net = NET_BROADCAST;
			dc.type = IMT_ALL;
			dc.id = IM_DEBUG_COMMAND;
			IMessage(&dc);
			if (msgDc->async == (sIMessage_debugCommand::duringAsynchronous)) {
				for (int i = 0; i < msgDc->argc; i++)
					free((char*)msgDc->argv[i]);
				delete [] msgDc->argv;
			}

			return 0;
		}

		case IM_DEBUG_COMMAND:
			return coreDebugCommand(static_cast<sIMessage_debugCommand*> (msgBase));

		case Tables::IM::registerTable: 
		{
			Tables::IM::RegisterTable* rt = static_cast<Tables::IM::RegisterTable*>(msgBase);
			rt->table = tables.registerTable(Ctrl->getPlugin((tPluginId)msgBase->sender), rt->tableId, rt->tableOpts);
			return (bool)rt->table;
		}

		case Unique::IMC::getMainInstance:
			return (int)Unique::instance();

	
		case Konnekt::IM::getTests: {
			return Konnekt::getCoreTests(static_cast<IM::GetTests*>(msgBase));
			}

		case Konnekt::IM::runTests:
			return Konnekt::runCoreTests(static_cast<IM::RunTests*>(msgBase));


	}
	if (Ctrl) {
		Ctrl->setError(IMERROR_NORESULT);
	}
	return 0;
}


int Konnekt::coreDebugCommand(sIMessage_debugCommand * arg) {
	if (!arg || !arg->argc) return 0;
	CStdString cmd = arg->argv[0];
	cmd.MakeLower();
	if (cmd == "help") {
		IMDEBUG(DBG_COMMAND, "- Dostêpne polecenia:");
		IMDEBUG(DBG_COMMAND, " test [nazwa_testu]");
	} else if (cmd == "test") {
		return command_test(arg);
	} else if (cmd == "debug") {
		if (arg->argEq(1, "objects")) {
			Stamina::debugDumpObjects(Stamina::mainLogger);
		}
	}

	Unique::debugCommand(arg);
	Tables::debugCommand(arg);
	return 0;
}
