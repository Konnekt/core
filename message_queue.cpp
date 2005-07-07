#include "stdafx.h"
#include "messages.h"
#include "tables.h"
#include "contacts.h"
#include "plugins.h"
#include "imessage.h"


namespace Konnekt { namespace Messages {

	int msgSent=0;
	int msgRecv=0;
	int timerMsg = 0;

	VOID CALLBACK timerMsgProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime) {
		Tables::saveMsg();
		KillTimer(0 , timerMsg);
	}

	void postSaveMsg() {
		if (timerMsg) KillTimer(0 , timerMsg);
		timerMsg = SetTimer(0 , 0 , 500 , (TIMERPROC)timerMsgProc);
	}

	void updateMessage(int pos , cMessage * m) {
		Msg.lock(pos);
		Msg.setint(pos , MSG_NET , m->net);
		Msg.setint(pos , MSG_TYPE , m->type);
		Msg.setch(pos , MSG_FROMUID , m->fromUid);
		Msg.setch(pos , MSG_TOUID , m->toUid);
		Msg.setch(pos , MSG_BODY , m->body);
		Msg.setch(pos , MSG_EXT  , m->ext);
		Msg.setint(pos , MSG_FLAG , m->flag);
		Msg.setint(pos , MSG_ACTIONP , m->action.parent);
		Msg.setint(pos , MSG_ACTIONI , m->action.id);
		Msg.setint(pos , MSG_NOTIFY , m->notify);
		Msg.set64(pos , MSG_TIME , m->time);
		Msg.unlock(pos);
	}

	int newMessage (cMessage * m , bool load , int pos) {
		int handler = -1;
		int r = 0;
		bool notinlist = false;
		// Rcv
		//if (*m->toUid) {m->flag |= MF_SEND;}

		if (!(m->flag & MF_SEND) && !(m->type & MT_MASK_NOTONLIST) && *m->fromUid && Contacts::findContact(m->net , m->fromUid)<0) {
			if (!IMessageDirect(Plug[0] , IMI_MSG_NOTINLIST , (int)m))
			{notinlist = true;handler=-1;goto messagedelete;}
		}
		// obs³u¿enie najpierw UI
		r = IMessageDirect(Plug[0],IM_MSG_RCV,(int)m);
		if (r & IM_MSG_delete) {handler=-1;goto messagedelete;}
		if (r & IM_MSG_ok) handler = 0;
		for (int i=Plug.size()-1; i>0 ;i--) {
			//    if (handler!=-1) break;
			m->id = 0;
			if (!(Plug[i].type & IMT_ALLMESSAGES)&&(!(Plug[i].type & IMT_MESSAGE) || (m->net && Plug[i].net && Plug[i].net!=m->net))) continue;
			r = IMessageDirect(Plug[i],IM_MSG_RCV,(int)m);
			if (r & IM_MSG_delete) {handler=-1;goto messagedelete;} // Wiadomosc nie zostaje dodana
			if (r & IM_MSG_ok) {
				handler = /*(Plug[i].type & IMT_MSGUI && !(m->flag & MF_SEND)) ? 0 :*/ i;
			}
		}
		if (m->flag & MF_HANDLEDBYUI) handler=0;
messagedelete:
		if (handler==-1) {
			// Zapisywanie w historii
			if (m->type == MT_MESSAGE && !(m->flag & MF_DONTADDTOHISTORY)) {
				sHISTORYADD ha;
				ha.m = m;
				//      ha.dir = m->flag & MF_SEND? "deleted" : "deleted";
				ha.dir = "deleted";
				ha.cnt = 0;
				ha.name = m->flag & MF_SEND? "nie wys³ane" : (notinlist?"spoza listy":"nie obs³u¿one");
				ha.session = 0;
				IMessageDirect(Plug[0] , IMI_HISTORY_ADD , (int)&ha);
			}  
			IMLOG("_Wiadomosc bez obslugi lub usunieta - %.50s...",m->body);
			if (load) Msg.deleterow(pos);
			return 0/*pos-1*/;
		}
		// Zapisywanie
		if (!load) {pos = Msg.addrow();
		//    } else {
		//    if ((int)m->id > MsgID) MsgID = m->id;
		}
		m->id = DT_MASKID(Msg.getint(pos , DT_C_ID));
		int cntID = m->type&MT_MASK_NOTONLIST?0:IMessage(IMC_FINDCONTACT,0,0,m->net,(int)m->fromUid);
		if (cntID != -1) Cnt.setint(cntID , CNT_LASTMSG , m->id);
		IMLOG("- Wiadomoœæ %d %s" , m->id , load?"jest w kolejce":"dodana do kolejki");
		Msg.lock(pos);
		updateMessage(pos , m);
		Msg.setint(pos , MSG_ID , m->id);
		Msg.setint(pos , MSG_HANDLER , handler);
		Msg.unlock(pos);
		//  if ((m->action.id || m->notify)) {IMessageDirect(Plug[0],IMI_NOTIFY,
		//           );}
		postSaveMsg();
		// stats
		if (!load && m->type==MT_MESSAGE)
			if (m->flag & MF_SEND) msgSent++;
			else msgRecv++;
			return m->id;
	}

	cMessage makeMessage(int pos, bool dup) {
		cMessage m;
		Msg.lock(pos);
		m.id=Msg.getint(pos , MSG_ID);
		m.net=Msg.getint(pos , MSG_NET);
		m.type=Msg.getint(pos , MSG_TYPE);
		m.fromUid=Msg.getch(pos , MSG_FROMUID);
		m.toUid=Msg.getch(pos , MSG_TOUID);
		m.body=Msg.getch(pos , MSG_BODY);
		m.ext=Msg.getch(pos , MSG_EXT);
		if (dup) {
			m.fromUid=strdup(m.fromUid);
			m.toUid=strdup(m.toUid);
			m.body=strdup(m.body);
			m.ext=strdup(m.ext);
		}
		m.flag=Msg.getint(pos , MSG_FLAG);
		m.action.parent=Msg.getint(pos , MSG_ACTIONP);
		m.action.id=Msg.getint(pos , MSG_ACTIONI);
		m.notify=Msg.getint(pos , MSG_NOTIFY);
		m.time=Msg.get64(pos , MSG_TIME);
		Msg.unlock(pos);
		return m;
	}

	bool isThatMessage(int i , sMESSAGESELECT * mw , unsigned int & position) {
		Msg.lock(i);
		int flag = Msg.getint(i , MSG_FLAG);
		int type = Msg.getint(i , MSG_TYPE);
		char * uid = flag & MF_SEND? Msg.getch(i , MSG_TOUID):Msg.getch(i , MSG_FROMUID);

		int r =
			(!mw->type || mw->type==-1 || ((unsigned int)Msg.getint(i , MSG_TYPE) == mw->type))
			&& (mw->net==NET_BC || (Msg.getint(i , MSG_NET) == mw->net) || (!mw->net && type&MT_MASK_NOTONLIST))
			&& (!mw->uid||!strcmp(uid,mw->uid) || (!*mw->uid && type&MT_MASK_NOTONLIST))
			&& ((flag & mw->wflag) == mw->wflag)
			&& (!(flag & mw->woflag))
			&& (mw->id==-1||mw->id==0||Msg.getint(i,MSG_ID)==mw->id)
			;

		/*  IMLOG("IsThat?[%d] i=%d , %d==%d , %d==%d , %s==%s , 0x%x==0x%x , !0x%x flag=0x%x",
		r , i , mw->type,Msg.getint(i , MSG_TYPE)
		, mw->net , Msg.getint(i , MSG_NET)
		, mw->uid , uid
		, mw->wflag , flag & mw->wflag , flag & mw->woflag , flag);
		*/        
		if (r) {
			if (mw->s_size == sizeof(sMESSAGESELECT) && position < mw->position)
				r = false;
			position ++;
		}
		Msg.unlock(i);
		return r;
	}


	void runMessageQueue(sMESSAGESELECT * ms, bool notifyOnly) {
		if (!ms) return;
		IMLOG("*MessageQueue - inQ=%d , reqNet=%d , reqType=%d" , Msg.getrowcount() , ms->net , ms->type);
		//  stack <pair <int , string> > notify;
		unsigned int siz = Msg.getrowcount();
		map <int , bool> notify_blank;
		unsigned int i=0;
		unsigned int count = 0;
		cMessage * m = 0;
		while (i < Msg.getrowcount()) {
			if (m) messageFree(m, false);
			m = 0;
			int id = Msg.getrowid(i);
			int r;
			if (Msg.getint(id , MSG_FLAG) & MF_PROCESSING) {i++;continue;}
			/*    if (find(notify.begin() , notify.end() , cntId)!=notify.end())
			notify.push_back(cntId);
			*/
			if (!isThatMessage(id , ms , count)) {i++; continue;}

			m = &makeMessage(id, true);
			int cntID = ((m->type & MT_MASK_NOTONLIST) || (!*m->fromUid && !*m->toUid))?0:
			Contacts::findContact(m->net , (m->flag & MF_SEND)?m->toUid:m->fromUid);

			if ((ms->net!=NET_BC && ms->type>0)?
				ms->net != m->net && ms->type != m->type
				:
			(m->flag & MF_REQUESTOPEN && (ms->id != m->id))) {i++;continue;}

			Msg.setint(id , MSG_FLAG , m->flag | MF_PROCESSING);
			r = IM_MSG_ok;
			if (!(m->flag & MF_OPENED) && !notifyOnly) {
				if (m->flag & MF_SEND) { // Wysylamy
					r = IMessageDirect(Plug[Msg.getint(id , MSG_HANDLER)],IM_MSG_SEND,(int)m);
				} else {
					// sprawdzanie listy
					//      IMLOG("CHECK %d %s = %d" , m->net , m->fromUid , CFindContact(m->net , m->fromUid));
					if (!(m->type & MT_MASK_NOTONLIST) && *m->fromUid && Contacts::findContact(m->net , m->fromUid)<0)
						r=IM_MSG_delete;  // Jezeli jest spoza listy powinna zostac usunieta
					//          IMessageDirect(Plug[0] , IMI_MSG_NOTINLIST , (int)m);
					else
						r = IMessageDirect(Plug[Msg.getint(id , MSG_HANDLER)],IM_MSG_OPEN,(int)m);
				}
			} // MF_OPENED
			if (r & IM_MSG_delete) {
				IMLOG("_Wiadomosc obsluzona - %d r=%x",Msg.getint(id , MSG_ID) , r);
				//      string test = m->fromUid;
				//      test = notify.top().second;
				//      test = notify.top().second.c_str();
				//      notify.push(make_pair(m->net , m->fromUid));
				if (cntID != -1) {SETCNTI(cntID , CNT_NOTIFY , NOTIFY_AUTO);}
				Msg.deleterow(id);
				continue;
			}
			else if (r & IM_MSG_update) {
				updateMessage(id , m);
			}
			if (!(r & IM_MSG_processing)) {
				m->flag&=~MF_PROCESSING;
				Msg.setint(id , MSG_FLAG , m->flag);
			}
			if (cntID != -1 && m->notify) {
				SETCNTI(cntID , CNT_NOTIFY , m->notify);
				SETCNTI(cntID , CNT_NOTIFY_MSG , m->id);
				SETCNTI(cntID , CNT_ACT_PARENT , m->action.parent);
				SETCNTI(cntID , CNT_ACT_ID , m->action.id);
			}
			i++;
		}
		if (m) messageFree(m, false);
		m = 0;
		IMessageDirect(Plug[0],IMI_NOTIFY,NOTIFY_AUTO);
		//notify.clear();
		if (siz != Msg.getrowcount())   postSaveMsg();
		return;
	}

	int messageNotify(sMESSAGENOTIFY * mn) {
		int i = Msg.getrowcount()-1;
		mn->action = sUIAction(0,0);
		mn->notify = 0;
		mn->id = 0;
		int found=0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == NET %d UID %s" , mn->net , mn->uid , Msg.getint(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
			if ((Msg.getint(i , MSG_NET) == mn->net && !strcmp(Msg.getch(i , MSG_FROMUID),mn->uid))
				|| (!mn->net && !*mn->uid && Msg.getint(i,MSG_TYPE)&MT_MASK_NOTONLIST)
				)
			{
				mn->action.parent = Msg.getint(i , MSG_ACTIONP);
				mn->action.id = Msg.getint(i , MSG_ACTIONI);
				mn->notify = Msg.getint(i , MSG_NOTIFY);
				mn->id = Msg.getint(i , MSG_ID);
				found++;
				if (mn->action.id || mn->notify) return found;
			}
			i--;
		}
		return found;
	}


	int messageWaiting(sMESSAGESELECT * mw) {
		int i = Msg.getrowcount()-1;
		int found=0;
		unsigned int count = 0;
		//  if (!mw->uid[0]) return 0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == TYPE %d NET %d UID %s" , mw->net , mw->uid ,  Msg.getint(i , MSG_TYPE) , Msg.getint(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
			//    int flag = Msg.getint(i , MSG_FLAG);
			if (isThatMessage(i , mw , count))
			{
				found++;
			}
			i--;
		}
		return found;
	}

	int getMessage(sMESSAGEPOP * mp , cMessage * m) {
		int i = Msg.getrowcount()-1;
		unsigned int count = 0;
		//  if (!mw->uid[0]) return 0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == TYPE %d NET %d UID %s" , mw->net , mw->uid ,  Msg.getint(i , MSG_TYPE) , Msg.getint(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
			if (isThatMessage(i , mp , count))
			{
				memcpy(m , &makeMessage(i, false) , sizeof(cMessage));
				return 1;
			}
			i--;
		}
		return 0;

	}

	int removeMessage(sMESSAGEPOP * mp , unsigned int limit) {
		if (!limit) limit=1;
		vector <pair <int , string> > notify;

		unsigned int c = 0;
		int i = Msg.getrowcount()-1;
		unsigned int count = 0;
		while (i>=0 && c<limit) {
			/*          if (Msg.getint(i,MSG_TYPE)&MT_MASK_NOTONLIST)
			notify.push_back(make_pair(0 , ""));
			else
			notify.push_back(make_pair(Msg.getint(i,MSG_NET) , Msg.getch(i,MSG_FROMUID)));
			*/
			if (isThatMessage(i , mp , count))
			{
				int cntID = Msg.getint(i,MSG_TYPE)&MT_MASK_NOTONLIST ? 0: IMessage(IMC_FINDCONTACT ,0,0, Msg.getint(i,MSG_NET) , Msg.getint(i,MSG_FROMUID));
				if (cntID != -1) SETCNTI(cntID , CNT_NOTIFY , NOTIFY_AUTO);
				//if (!(Msg.getint(i,MSG_FLAG)&MF_SEND))
				Msg.deleterow(i);
				c++;
				//        return 1;
			}
			i--;
		}
		/*  for (vector <pair <int , string> >::iterator notify_it = notify.begin() ; notify_it!=notify.end(); notify_it++) {
		IMessageDirect(Plug[0],IMI_NOTIFY,IMessage(IMC_FINDCONTACT,0,0,notify_it->first,(int)(notify_it->second.c_str())));
		}
		*/
		IMessageDirect(Plug[0],IMI_NOTIFY,NOTIFY_AUTO);

		return c;

	}


	void messageProcessed(int id , bool remove) {
		id = DT_MASKID(id);
		Msg.setint(id , MSG_FLAG , Msg.getint(id , MSG_FLAG) & (~MF_PROCESSING));
		if (remove) {
			sMESSAGEPOP mp;
			mp.id = id;
			removeMessage(&mp , 1);
		}
	}

	/*
	int CMessagePop(sMESSAGEPOP * mp , cMessage * m) {
	int r = CMessageGet(mp , m);
	if (r) CMessageRemove(mp , 1);
	return r;
	}
	*/
	void initMessages(void) {
		unsigned int i=0;
		Msg.lock(DT_LOCKALL);
		while (i < Msg.getrowcount()) {
			if (newMessage(&makeMessage(i, false),1,i)) i++;
		}
		Msg.unlock(DT_LOCKALL);
	}

};};