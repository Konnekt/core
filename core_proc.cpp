/*

Obs³uga "rdzeniowych" komunikatów

*/

#include "stdafx.h"
#include "main.h"
#include "imessage.h"
#include "konnekt_sdk.h"
#include "threads.h"
#include "profiles.h"
#include "messages.h"
#include "plugins.h"
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

#include <Stamina\Debug.h>


//  IMessage do Core
int Konnekt::coreIMessageProc(sIMessage_base * msgBase) {
	sIMessage_2params * msg = static_cast<sIMessage_2params*>(msgBase);
	if (msgBase->id == IMC_LOG) {
		return 0;
	}  
	TLSU().lastIM.enterMsg(msgBase,0);
	int a , i;
	char * ch , * ch2;
	string str;
	switch (msg->id) {
		case IMC_ISNEWVERSION: return newVersion;
		case IMC_ISNEWPROFILE: return newProfile;
		case IMC_ISWINXP: return isComCtl6;
		case IMC_THREADSTART: 
			Ctrl->onThreadStart();
			return 1;//IMessage(IM_THREADSTART , NET_BC , -1 , 0 , 0 , 0);
		case IMC_THREADEND: 
			Ctrl->onThreadEnd();
			return 1;//IMessage(IM_THREADEND , NET_BC , -1 , 0 , 0 , 0);

		case IMC_RESTORECURDIR: _chdir(appPath.c_str()); return (int)appPath.c_str();
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
			IMLOG("- IMC_SHUTDOWN (od %s) ... " , Plug.Name(msg->sender));
			if (!msg->p1 && (!canQuit || IMessageSum(IM_CANTQUIT , IMT_ALL , 0 , 0 , 0))) {
				IMLOG("- IMC_SHUTDOWN anulowany");
				return 0;
		 }
			running = false;
			if (msg->p1)
				canWait = false; // natychmiastowy shutDown...
			//end();
			//deInit();
			//PostMessage(hwndDebug , WM_CLOSE,0,0);
			//DestroyWindow(hwndDebug);
			//         #ifdef __DEBUG
			//         debug = false;
			//         #endif
			//         if (msg->p1) 
			deInit(true);
			//         else PostQuitMessage(0);
			return 1;}
		case IMC_NET_TYPE: {
			int type = 0;
			for (int i=0; i<Plug.size(); i++)
				if (Plug[i].net==msg->p1) type |= Plug[i].type;
			return type;}
		case IMC_DISCONNECT: ISRUNNING();IMessage(IM_DISCONNECT , -1 , IMT_PROTOCOL); return 0;
		case IMC_NEWMESSAGE: 
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::newMessage((cMessage *)msg->p1);
		case IMC_MESSAGEACK: ISRUNNING(); 
			if (((cMessageAck*)msg->p1)->flag & MACK_NOBROADCAST) 
				IMessageDirect(Plug[0] , IM_MSG_ACK , msg->p1 , 0); 
			else
				IMessage(IM_MSG_ACK , NET_BC , IMT_MESSAGEACK , msg->p1 , 0 , 0); 
			return true;
		case IMC_MESSAGEQUEUE:
			ISRUNNING();
			IMESSAGE_TS();
			//          if (1/*GetCurrentThreadId()!=curThread*/) PostThreadMessage(curThread , MYWM_MESSAGEQUEUE , 0 , 0);
			//            else CMessageQueue();
			Messages::runMessageQueue((sMESSAGESELECT*)msg->p1 , msg->p2);
			return 0;
		case IMC_MESSAGENOTIFY: return Messages::messageNotify((sMESSAGENOTIFY *)msg->p1);
		case IMC_MESSAGEWAITING: return Messages::messageWaiting((sMESSAGEWAITING *)msg->p1);
		case IMC_MESSAGEREMOVE:
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::removeMessage((sMESSAGEWAITING *)msg->p1,msg->p2);
			//       case IMC_MESSAGEPOP: return CMessagePop((sMESSAGEWAITING *)msg->p1 , (cMessage*)msg->p2);
		case IMC_MESSAGEGET: 
			ISRUNNING();
			IMESSAGE_TS();
			return Messages::getMessage((sMESSAGEWAITING *)msg->p1 , (cMessage*)msg->p2);
		case IMC_MESSAGEPROCESSED: 
			ISRUNNING();
			IMESSAGE_TS();
			Messages::messageProcessed(msg->p1 , msg->p2); return 1;
		case IMC_FINDCONTACT: return Contacts::findContact(msg->p1,(char *)msg->p2);
			/*       case IMC_GETCNTI: return Cnt.getint(msg->p1 , msg->p2);
			case IMC_GETCNTC: return (int)Cnt.getch(msg->p1 , msg->p2);
			case IMC_SETCNTI: psc = (sSETCNT*)msg->p2;
			if (!psc->mask)
			if (psc->typ == CNT_STATUS) psc->mask = CNTM_STATUS;
			else psc->mask = -1;
			if (psc->mask == -1)
			return Cnt.setint(msg->p1 , psc->typ,psc->val);
			else
			return Cnt.setint(msg->p1 , psc->typ, (Cnt.getint(msg->p1 , psc->typ) & (~psc->mask)) | (psc->val));
			case IMC_SETCNTC: return Cnt.setch(msg->p1 , ((sSETCNT*)msg->p2)->typ,(char*)(((sSETCNT*)msg->p2)->val));
			*/
		case IMC_CNT_CHANGED: {
			ISRUNNING();
			IMESSAGE_TS();
			sIMessage_CntChanged msgCC(msg);
			Contacts::updateContact(msgCC._cntID);
			if (Cnt.getint(msgCC._cntID,CNT_INTERNAL)&1) {
				Cnt.setint(msgCC._cntID,CNT_INTERNAL,Cnt.getint(msgCC._cntID,CNT_INTERNAL)& (~1));
				IMessage(IM_CNT_ADD,NET_BC,IMT_CONTACT,Cnt.getrowid(msgCC._cntID),0,0);
			}
			else {
				msgCC.id = IM_CNT_CHANGED;
				msgCC.net = NET_BC;
				msgCC.type = IMT_CONTACT;
				// Trzeba upewniæ siê, ¿e oba pola s¹ wype³nione
				if (msgCC._changed.net || msgCC._changed.uid) {
					if (!msgCC._oldUID) msgCC._oldUID = Cnt.getch(msgCC._cntID , CNT_UID);
					if (!msgCC._changed.net && msgCC._oldNet == NET_NONE) msgCC._oldNet = Cnt.getint(msgCC._cntID , CNT_NET);
				}
				IMessageProcess(&msgCC , 0);
			}
			break;}
		case IMC_CNT_IDEXISTS: return Cnt.idexists(msg->p1);         
		case IMC_CNT_ADD: {
			ISRUNNING();
			IMESSAGE_TS();
			int cnt = -1;
			if (msg->p1 && msg->p2 && *(char*)msg->p2) {
				int cnt = Contacts::findContact(msg->p1,(char *)msg->p2);
				if (cnt != -1) {
					int r = IMessageProcess(&sIMessage_msgBox(IMI_CONFIRM, stringf(resString(IDS_ASK_CNTOVERWRITE).c_str(), msg->p2, msg->p1, Cnt.getch(cnt, CNT_DISPLAY)).c_str(), "Dodawanie kontaktu", MB_YESNOCANCEL), 0);
					switch (r) {
		case IDNO:cnt=-1;break;
		case IDCANCEL:return -1;
					}
				}
			}
			if (cnt == -1) {
				cnt = Cnt.getrowid(Cnt.addrow());
				//if (!msg->p1) msg->p2 = (int)inttoch(Cnt.getint(a , DT_C_ID));
				if (msg->p1) Cnt.setint(cnt , CNT_NET , msg->p1);
				if (msg->p2) Cnt.setch (cnt , CNT_UID , (char*)msg->p2);
				Cnt.setint(cnt , CNT_INTERNAL , Cnt.getint(cnt , CNT_INTERNAL)|1);
				IMessage(IM_CNT_ADDING,NET_BC,IMT_CONTACT,cnt,0,0);
			}
			return cnt;}
		case IMC_CNT_REMOVE: ISRUNNING(); if (msg->p1>0 && (!msg->p2
								 || UIMessage(IMI_CONFIRM
								 ,(int)stringf(resString(IDS_ASK_CNTREMOVE).c_str(),Cnt.getch(msg->p1,CNT_DISPLAY)).c_str()
								 ,0))) {Messages::removeMessage(&sMESSAGESELECT(Cnt.getint(msg->p1,CNT_NET),Cnt.getch(msg->p1,CNT_UID),MT_MESSAGE,0,MF_SEND),-1);
			IMessage(IM_CNT_REMOVE,NET_BC,IMT_CONTACT,msg->p1,msg->p2,0);
			a = Cnt.deleterow(msg->p1);
			IMessage(IM_CNT_REMOVED,NET_BC,IMT_CONTACT,msg->p1,msg->p2,0);
			return a;} else return 0;
		case IMC_CNT_COUNT: return Cnt.getrowcount();
		case IMC_CFG_SETCOL: case IMC_CNT_SETCOL: {
			sSETCOL * setcol = reinterpret_cast<sSETCOL*>(msg->p1);
			sIMessage_setColumn sc(msgBase->id == IMC_CFG_SETCOL?DTCFG:DTCNT , setcol->id , setcol->type , setcol->def , setcol->name);
			sc.sender = msgBase->sender;
			return Tables::setColumn(&sc);
							 }
		case sIMessage_setColumn::__msgID:
			return Tables::setColumn(static_cast<sIMessage_setColumn*>(msgBase));

		case IMC_CFG_SAVE  : 
			ISRUNNING();
			//IMESSAGE_TS_NOWAIT(1);
			/*IMessage(IM_CFG_SAVE,-1,IMT_ALL,0,0,0);*/ /*saveProfile();*/
			return 1;

		case IMC_GETINSTANCE: return (int)hInst;
		case IMC_GETPROFILE: return (int)profile.c_str();
		case IMC_LOGDIR: return (int)Debug::logPath.c_str();
		case IMC_TEMPDIR: return (int)tempPath.c_str();
		case IMC_PLUG_COUNT: return Plug.size();
		case IMC_PLUG_HANDLE: return (int)Plug[msg->p1].hModule;
		case IMC_PLUG_ID: return (int)Plug[msg->p1].ID;
		case IMC_PLUG_FILE:  return (int)Plug[msg->p1].file.c_str();
		case IMC_PLUGID_POS: return Plug.FindID(msg->p1);
		case IMC_PLUGID_HANDLE: {int pos = Plug.FindID(msg->p1); return pos!=-1?(int)Plug[pos].hModule:0;}
		case IMC_FINDPLUG: {
			int pos = Plug.Find(msg->p1 , msg->p2 , 0);
			if (pos == -1) return 0;
			return Plug[pos].ID;
						   }
		case IMC_FINDPLUG_BYNAME: {
			if (!msg->p1) return 0;
			for (cPlugs::Plug_it_t Plug_it=Plug.Plug.begin(); Plug_it!=Plug.Plug.end(); Plug_it++)
				if (Plug_it->name == (char*)msg->p1) return Plug_it->ID;
			return 0;
								  }
		case IMC_FINDPLUG_BYSIG: {
			if (!msg->p1) return 0;
			for (cPlugs::Plug_it_t Plug_it=Plug.Plug.begin(); Plug_it!=Plug.Plug.end(); Plug_it++)
				if (Plug_it->sig == (char*)msg->p1) return Plug_it->ID;
			return 0;
								 }
		case sIMessage_plugOut::__msgID: {
			ISRUNNING();
			IMESSAGE_TS();
			sIMessage_plugOut * po = static_cast<sIMessage_plugOut*>(msgBase);
			int ID = Plug.FindID(po->_plugID);
			int senderID = Plug.FindID(po->sender);
			if (ID == -1 || senderID == -1) return 0;
			int pl = Plg.findby((TdEntry)Plug[ID].file.c_str() , PLG_FILE);
			if (pl < 0) return 0;
			if (po->_unload & sIMessage_plugOut::euNextStart) {
				Plg.setint(pl , PLG_LOAD , -1);
				Tables::savePlg();
			}
			CStdString namePlug = Plug[ID].GetName();
			CStdString nameSender = Plug[senderID].GetName();
			CStdString reason = po->_reason;
			if (po->_unload & sIMessage_plugOut::euNow) {
				Plug.PlugOUT(ID);
				Plug.Plug.erase(Plug.Plug.begin() + ID);
			}

			CStdString msg = resStr(IDS_INF_PLUGOUT);
			//Wtyczka %s od³¹czy³a wtyczkê %s\r\nMo¿esz j¹ przywróciæ rêcznie w opcji "wtyczki".\r\n\r\n%s\r\n\r\n%s
			CStdString restart = 
				po->_restart == sIMessage_plugOut::erAsk ? resStr(IDS_ASK_RESTART) 
				: po->_restart == sIMessage_plugOut::erYes ? resStr(IDS_INF_RESTART) 
				: po->_restart == sIMessage_plugOut::erAskShut ? resStr(IDS_ASK_SHUTDOWN) 
				: po->_restart == sIMessage_plugOut::erShut ? resStr(IDS_INF_SHUTDOWN) 
				: "";
			msg.Format(msg , nameSender , namePlug , reason , restart.c_str());
			IMLOG("plugOut\n%s" , msg.c_str());
			if (MessageBox(NULL , msg , resStr(IDS_APPNAME) , MB_ICONWARNING|MB_TASKMODAL|(po->_restart & sIMessage_plugOut::erAsk ? MB_YESNO : MB_OK)) == IDNO) 
				return 1;

			if (po->_restart >= sIMessage_plugOut::erAskShut)
				IMessage(IMC_SHUTDOWN , 0 , 0 , 1);
			else if (po->_restart >= sIMessage_plugOut::erAsk)
				IMessage(IMC_RESTART , 0 , 0);
			return 1;
										 }
		case IMC_STATUSCHANGE: {
			ISRUNNING();
			IMESSAGE_TS(0);
			sIMessage_StatusChange sc(msgBase);
			sc.net = NET_BC;
			sc.type = IMT_ALL;
			sc.id = IM_STATUSCHANGE;
			sc.plugID = sc.sender;
			sc.sender = 0;
			return IMessageProcess(&sc , 0);
		}

		case IMC_CNT_SETSTATUS: {
			ISRUNNING();       
			IMESSAGE_TS(0);
			sIMessage_StatusChange sc(msgBase);
			sc.net = NET_BC;
			sc.type = IMT_ALL;
			sc.id = IM_CNT_STATUSCHANGE;
			sc.sender = 0;
			IMessageProcess(&sc,0);
			if (sc.status != -1) Cnt.setint(sc.cntID , CNT_STATUS , (Cnt.getint(sc.cntID , CNT_STATUS) & ~ST_MASK) | sc.status);
			if (sc.info) Cnt.setch(sc.cntID , CNT_STATUSINFO , sc.info);
			return 0;
			}

		case IMC_ARGC: return _argc;
		case IMC_ARGV: return (int) _argv[msg->p1];

		case IMC_ISDEBUG: return Debug::superUser;
			//            #ifdef __DEBUG
			//            1
			//            #else
			//            0
			//            #endif
			//            ;
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
			setPlugins(true , false);
			PlugsDialog(); return 1;
		case IMC_CONNECTED:
			return Connections::isConnected();
			/* DONE : Sprawdzanie istnienia po³¹czenia */
		case IMC_VERSION: {
			FILEVERSIONINFO fvi;
			FileVersionInfo (app , (char*)msg->p1 , &fvi);
			return VERSION_TO_NUM(fvi.major , fvi.minor , fvi.release , fvi.build);}
		case IMC_PLUG_VERSION: {
			if (msg->p1 == -1) {return IMessage(IMC_VERSION , 0,0,msg->p2);}
			
			if (!Plug.exists(msg->p1)) return 0;
			if (msg->p2) {strcpy((char*)msg->p2 , Plug[msg->p1].version.c_str());}
			return Plug[msg->p1].versionNumber;
							   }
		case IMC_CNT_IGNORED:
			a = stristr(Cfg.getch(0,CFG_IGNORE) , STR(_sprintf("\n%d@%s\n",msg->p1,msg->p2))) ? 1 : 0 ;
			return a;
		case IMC_CNT_INGROUP:
			ch = (msg->p2?(char*)msg->p2 : Cfg.getch(0 , CFG_CURGROUP));
			if (!*ch) { // sprawdzamy grupê pust¹
				if (Cfg.getint(0, CFG_UIALLGROUPS_NOGROUP)) {
					return !*Cnt.getch(msg->p1 , CNT_GROUP) ? 1 : 0; // wpuszczamy tylko puste...
				} else
					return 1; // wszystkie tu pasuj¹
			}
			return !stricmp(Cnt.getch(msg->p1 , CNT_GROUP) , ch);
		case IMC_IGN_ADD:
			ISRUNNING();
			IMESSAGE_TS();
			if (!UIMessage(IMC_IGN_FIND , msg->p1 , msg->p2)) {
				ch = Cfg.getch(0,CFG_IGNORE);
				Cfg.setch(0 , CFG_IGNORE , STR(_sprintf("%s%s%d@%s\n" , *ch?"":"\n" , ch , msg->p1 , msg->p2)));
				Contacts::updateContact(Contacts::findContact(msg->p1 , (char*)msg->p2));
				IMessage(IM_IGN_CHANGED , NET_BC , IMT_CONTACT , msg->p1 , msg->p2 , 0);
				return 1;
			}
			return 0;
		case IMC_IGN_DEL: {
			ISRUNNING();
			IMESSAGE_TS();
			const char * item = _sprintf("\n%d@%s\n", msg->p1 , msg->p2);
			if ((ch2 = stristr(Cfg.getch(0,CFG_IGNORE) , STR(item)))!=0) {
				strcpy(ch2 , ch2 + strlen(item)-1);
				Contacts::updateContact(Contacts::findContact(msg->p1 , (char*)msg->p2));
				IMessage(IM_IGN_CHANGED , NET_BC , IMT_CONTACT , -msg->p1 , msg->p2 , 0);
			}
			return 0;}


		case IMC_GRP_FIND:
			a = stristr(Cfg.getch(0,CFG_GROUPS) , STR(_sprintf("\n%s\n",msg->p1))) ? 1 : 0 ;
			return a;
		case IMC_GRP_ADD:
			ISRUNNING(); a=0;
			IMESSAGE_TS();
			if (!UIMessage(IMC_GRP_FIND , msg->p1)) {
				a = 1;
				ch = Cfg.getch(0,CFG_GROUPS);
				Cfg.setch(0 , CFG_GROUPS , STR(_sprintf("%s%s%s\n" , *ch?"":"\n" , ch , msg->p1)));
				IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0 , 0);
			}
			return a;
		case IMC_GRP_DEL:
			ISRUNNING();
			IMESSAGE_TS();
			if ((ch2 = stristr(Cfg.getch(0,CFG_GROUPS) , STR(_sprintf("\n%s\n",msg->p1))))!=0) {
				strcpy(ch2 , ch2 + strlen((char*)msg->p1)+1);
				for (i = 1; i < (int)Cnt.getrowcount() ; i++) {
					if (!stricmp((char*)msg->p1 , Cnt.getch(i,CNT_GROUP))) {
						Cnt.setch(i , CNT_GROUP , "");
						sIMessage_CntChanged cc(IM_CNT_CHANGED , i);
						cc._changed.group = true;
						Ctrl->IMessage(&cc);
					}
				}
				IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0 , 0);
			}
			return 0;
		case IMC_GRP_RENAME:
			ISRUNNING();
			IMESSAGE_TS();
			if (!UIMessage(IMC_GRP_FIND , msg->p1) || UIMessage(IMC_GRP_FIND , msg->p2)) return 0;
			//            IMLOG("GRP_RENAME %s %s" , msg->p1 , msg->p2);
			// Usuwamy
			if ((ch2 = stristr(Cfg.getch(0,CFG_GROUPS) , STR(_sprintf("\n%s\n",msg->p1))))!=0) {
				strcpy(ch2 , ch2 + strlen((char*)msg->p1)+1);
			}
			// Dodajemy
			ch = Cfg.getch(0,CFG_GROUPS);
			Cfg.setch(0 , CFG_GROUPS , STR(_sprintf("%s%s%s\n" , *ch?"":"\n" , ch , msg->p2)));
			for (i = 1; i < (int)Cnt.getrowcount() ; i++) {
				if (!stricmp((char*)msg->p1 , Cnt.getch(i,CNT_GROUP))) {
					Cnt.setch(i , CNT_GROUP , (char*)msg->p2);
					sIMessage_CntChanged cc(IM_CNT_CHANGED , i);
					cc._changed.group = true;
					Ctrl->IMessage(&cc);
				}
			}
			IMessage(IM_GRP_CHANGED , NET_BC , IMT_CONTACT , 0 , 0 , 0);
			return 0;

		case IMC_PROFILEDIR:
			return (int)profileDir.c_str();
		case IMC_PROFILESDIR:
			return (int)profilesDir.c_str();
		case IMC_RESTART:
			ISRUNNING();
			IMESSAGE_TS();
			deInit(false);
			restart();
			//            IMCore(IMC_SHUTDOWN , 0 , 0 , 0);
			return 1;
		case IMC_PROFILECHANGE:{
			ISRUNNING();
			IMESSAGE_TS();
			if (access((profilesDir+(char*)msg->p1).c_str() , 0)) {
				if (!msg->p2) return 0;
				else
					mkdir((profilesDir+(char*)msg->p1).c_str());
			}
			//            fprintf(logFile , "\n_____________________________________________\n\nZmiana profilu na %s [%d]\n_____________________________________________\n\n" , msg->p1 , msg->p2);
			profile = (char*)msg->p1;
			// trzeba wywyaliæ -profile z parametrów...
			char * argProfile = (char*)getArgV(ARGV_PROFILE , false , 0);
			if (argProfile) {
				*argProfile = 0;
			}
			deInit(false);
			restart();
			//            Init();
			//            IMCore(IMC_SHUTDOWN , 0 , 0 , 0);
			return 1;}
		case IMC_PROFILEREMOVE:
			ISRUNNING();
			IMESSAGE_TS();
			profile = "?";
			//            atexit_remove();
			deInit(false);
			remove();
			//            IMCore(IMC_SHUTDOWN , 0 , 0 , 0);
			return 0;
		case IMC_PROFILEPASS:
			ISRUNNING();
			profilePass();
			return 0;
		case IMC_CFG_CHANGED:
			ISRUNNING();
			IMESSAGE_TS();
			IMessage(IM_CFG_CHANGED , NET_BC , IMT_CONFIG , 0 , 0 , 0);
			return 0;
		case IMC_SAVE_CFG:
			ISRUNNING();
			IMESSAGE_TS();
			Tables::saveCfg();
			return 1;
		case IMC_SAVE_CNT:
			ISRUNNING();
			IMESSAGE_TS();
			Tables::saveCnt();
			return 1;
		case IMC_SAVE_MSG:
			ISRUNNING();
			IMESSAGE_TS();
			Tables::saveMsg();
			return 1;
		case IMC_SETCONNECT:
			ISRUNNING();
			IMESSAGE_TS_NOWAIT(0); /// <---- tu by³ NOWAIT!
			Connections::setConnect(msg->sender, msg->p1);
			return 0;
		case IMC_GETMSGCOLDESC:
			return (int)&Msg.cols;
		case IMC_GETMD5DIGEST:
			if (!msg->p1 || !md5digest[0]) return 0;
			memcpy(&msg->p1 , md5digest , 16);
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
		case IMC_HINTERNET_OPEN: {
			HINTERNET h;
			CStdString proxy =  Cfg.getint(0,CFG_PROXY) ? (Cfg.getch(0,CFG_PROXY_HOST) + string(":") + (Cfg.getint(0,CFG_PROXY_PORT) ? inttoch(Cfg.getint(0,CFG_PROXY_PORT)) : "8080")) : "";
			h = InternetOpen(msg->p1 ? (char*)msg->p1 : "Konnekt", proxy.empty() ? INTERNET_OPEN_TYPE_DIRECT : INTERNET_OPEN_TYPE_PROXY , proxy.empty()? 0 : proxy.c_str(), 0, 0);
			if (!proxy.empty() && Cfg.getint(0,CFG_PROXY_AUTH)) {
				InternetSetOption(h, INTERNET_OPTION_PROXY_USERNAME,
					Cfg.getch(0,CFG_PROXY_LOGIN), strlen(Cfg.getch(0,CFG_PROXY_LOGIN))+1);
				InternetSetOption(h, INTERNET_OPTION_PROXY_PASSWORD,
					Cfg.getch(0,CFG_PROXY_PASS), strlen(Cfg.getch(0,CFG_PROXY_PASS))+1);
			}


			return (int)h;}
		case IMC_GET_MAINTHREAD:
			return (int) hMainThread;
		case IMC_DEBUG_COMMAND: {
			sIMessage_debugCommand * msgDc = static_cast<sIMessage_debugCommand*> (msgBase);
			if (msgDc->async == sIMessage_debugCommand::asynchronous) {
				const char ** argv = new const char * [msgDc->argc];
				for (int i = 0; i < msgDc->argc; i++)
					argv[i] = strdup(msgDc->argv[i]);
				sIMessage_debugCommand dc (msgDc->argc, argv, sIMessage_debugCommand::duringAsynchronous);
				return Ctrl->RecallIMTS(0, false, &dc, 0);
			}
			IMESSAGE_TS();
			coreDebugCommand(msgDc);
			sIMessage_debugCommand dc = *msgDc;
			dc.net = NET_BROADCAST;
			dc.type = IMT_ALL;
			dc.id = IM_DEBUG_COMMAND;
			IMessageProcess(&dc, 0);
			if (msgDc->async == (sIMessage_debugCommand::duringAsynchronous)) {
				for (int i = 0; i < msgDc->argc; i++)
					free((char*)msgDc->argv[i]);
				delete [] msgDc->argv;
			}

			return 0;}
		case Unique::IM::addRange: {
			Unique::IM::_addRange * ar = static_cast<Unique::IM::_addRange *>(msgBase);
			return Unique::addRange(ar->domainId, *ar->range, ar->setAsDefault);}

		case Unique::IM::domainExists: {
			Unique::IM::_rangeIM * ri = static_cast<Unique::IM::_rangeIM *>(msgBase);
			return Unique::domainExists(ri->domainId);}
		case Unique::IM::registerId: {
			Unique::IM::_rangeIM * ri = static_cast<Unique::IM::_rangeIM *>(msgBase);
			return Unique::registerId(ri->domainId, ri->identifier, ri->name);}
		case Unique::IM::registerName: {
			Unique::IM::_rangeIM * ri = static_cast<Unique::IM::_rangeIM *>(msgBase);
			return Unique::registerName(ri->domainId, ri->name, ri->rangeId);}
		case Unique::IM::removeDomain: {
			Unique::IM::_rangeIM * ri = static_cast<Unique::IM::_rangeIM *>(msgBase);
			return Unique::removeDomain(ri->domainId);}
		case Unique::IM::unregister: {
			Unique::IM::_rangeIM * ri = static_cast<Unique::IM::_rangeIM *>(msgBase);
			return Unique::unregister(ri->domainId, ri->identifier);}

		case Tables::IM::registerTable: {
			Tables::IM::_registerTable* rt = static_cast<Tables::IM::_registerTable*>(msgBase);
			rt->table = tables.registerTable(Ctrl->getPlugin((tPluginId)msgBase->sender), rt->tableId, rt->tableOpts);
			return (bool)rt->table;
			}

	}
	TLSU().setError(IMERROR_NORESULT);
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
		command_test(arg);
	} else if (cmd == "debug") {
		if (arg->argEq(1, "objects")) {
			Stamina::debugDumpObjects(Stamina::mainLogger);
		}
	}

	Unique::debugCommand(arg);
	Tables::debugCommand(arg);
	return 0;
}
