#include "stdafx.h"
#include "messages.h"
#include "tables.h"
#include "contacts.h"
#include "plugins.h"
#include "imessage.h"
#include "threads.h"

using namespace Stamina;
using namespace Tables;

namespace Konnekt { namespace Messages {

	int msgSent=0;
	int msgRecv=0;
	int timerMsg = 0;


	void updateMessage(int pos , cMessage * m) {
		oTableImpl msg(tableMessages);
		msg->lockData(pos);
		msg->setInt(pos , MSG_NET , m->net);
		msg->setInt(pos , MSG_TYPE , m->type);
		msg->setString(pos , MSG_FROMUID , m->fromUid);
		msg->setString(pos , MSG_TOUID , m->toUid);
		msg->setString(pos , MSG_BODY , m->body);
		msg->setString(pos , MSG_EXT  , m->ext);
		msg->setInt(pos , MSG_FLAG , m->flag);
		msg->setInt(pos , MSG_ACTIONP , m->action.parent);
		msg->setInt(pos , MSG_ACTIONI , m->action.id);
		msg->setInt(pos , MSG_NOTIFY , m->notify);
		msg->setInt64(pos , MSG_TIME , m->time);
		msg->unlockData(pos);
	}

	int newMessage (cMessage * m , bool load , int pos) {
		int handler = -1;
		int r = 0;
		bool notinlist = false;
		// Rcv
		//if (*m->toUid) {m->flag |= MF_SEND;}

		if (!(m->flag & MF_SEND) && !(m->type & MT_MASK_NOTONLIST) && *m->fromUid && Contacts::findContact(m->net , m->fromUid)<0) {
			if (!ICMessage(IMI_MSG_NOTINLIST , (int)m)) {
				notinlist = true;
				handler=-1;
				goto messagedelete;
			}
		}
		// obs³u¿enie najpierw UI
		r = plugins[pluginUI].IMessageDirect(IM_MSG_RCV,(int)m, 0);
		if (r & IM_MSG_delete) {handler=-1;goto messagedelete;}
		if (r & IM_MSG_ok) handler = Konnekt::pluginUI;
		for (Plugins::tList::reverse_iterator it = plugins.rbegin(); it != plugins.rend(); ++it) {
			Plugin& plugin = **it;
			if (plugin.getId() == pluginUI) break;

			m->id = 0;
			if (!(plugin.getType() & IMT_ALLMESSAGES) && (!(plugin.getType() & IMT_MESSAGE) || (m->net && plugin.getNet() && plugin.getNet() != m->net))) continue;
			r = plugin.IMessageDirect(IM_MSG_RCV,(int)m, 0);
			if (r & IM_MSG_delete) {handler=-1;goto messagedelete;} // Wiadomosc nie zostaje dodana
			if (r & IM_MSG_ok) {
				handler = plugins.getIndex(plugin.getId());
			}
		}
		if (m->flag & MF_HANDLEDBYUI) handler = Konnekt::pluginUI;
messagedelete:

		oTableImpl msg(tableMessages);
		msg->lateSave(false);

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
				plugins[pluginUI].IMessageDirect(IMI_HISTORY_ADD , (int)&ha, 0);
			}  
			IMLOG("_Wiadomosc bez obslugi lub usunieta - %.50s...",m->body);
			if (load) msg->removeRow(pos);
			return 0/*pos-1*/;
		}
		// Zapisywanie
		if (!load) {pos = msg->addRow();
		//    } else {
		//    if ((int)m->id > MsgID) MsgID = m->id;
		}
		m->id = msg->getRowId(pos);
		int cntID = m->type&MT_MASK_NOTONLIST?0:ICMessage(IMC_FINDCONTACT,m->net,(int)m->fromUid);
		if (cntID != -1) cnt->setInt(cntID , CNT_LASTMSG , m->id);
		IMLOG("- Wiadomoœæ %d %s" , m->id , load?"jest w kolejce":"dodana do kolejki");
		{
			TableLocker lock(msg, pos);
			updateMessage(pos , m);
			msg->setInt(pos , MSG_ID , m->id);
			msg->setInt(pos , MSG_HANDLER , handler);
		}
		//  if ((m->action.id || m->notify)) {IMessageDirect(Plug[0],IMI_NOTIFY,
		//           );}
		msg->lateSave(true);
		// stats
		if (!load && m->type==MT_MESSAGE)
			if (m->flag & MF_SEND) msgSent++;
			else msgRecv++;
			return m->id;
	}

	cMessage makeMessage(int pos, bool dup) {
		cMessage m;
		oTableImpl msg(tableMessages);
		TableLocker lock(msg, pos);
		m.id = msg->getInt(pos , MSG_ID);
		m.net = msg->getInt(pos , MSG_NET);
		m.type = msg->getInt(pos , MSG_TYPE);

		String& mbFromUid = TLSU().buffer().getString(true);
		String& mbToUid = TLSU().buffer().getString(true);
		String& mbBody = TLSU().buffer().getString(true);
		String& mbExt = TLSU().buffer().getString(true);
		mbFromUid = msg->getString(pos , MSG_FROMUID);
		mbToUid = msg->getString(pos , MSG_TOUID);
		mbBody = msg->getString(pos , MSG_BODY);
		mbExt = msg->getString(pos , MSG_EXT);

		m.fromUid = (char*)mbFromUid.c_str();
		m.toUid = (char*)mbToUid.c_str();
		m.body = (char*)mbBody.c_str();
		m.ext = (char*)mbExt.c_str();
		if (dup) {
			m.fromUid = _strdup(m.fromUid);
			m.toUid = _strdup(m.toUid);
			m.body = _strdup(m.body);
			m.ext = _strdup(m.ext);
		}
		m.flag=msg->getInt(pos , MSG_FLAG);
		m.action.parent=msg->getInt(pos , MSG_ACTIONP);
		m.action.id=msg->getInt(pos , MSG_ACTIONI);
		m.notify=msg->getInt(pos , MSG_NOTIFY);
		m.time=msg->getInt64(pos , MSG_TIME);
		return m;
	}

	bool isThatMessage(int i , sMESSAGESELECT * mw , unsigned int & position) {
		oTableImpl msg(tableMessages);
		TableLocker lock(msg, i);
		int flag = msg->getInt(i , MSG_FLAG);
		int type = msg->getInt(i , MSG_TYPE);
		string uid;
		if (flag & MF_SEND) {
			uid = msg->getString(i , MSG_TOUID);
		} else {
			uid = msg->getString(i, MSG_FROMUID);
		}

		int r =
			(!mw->type || mw->type==-1 || ((unsigned int)msg->getInt(i , MSG_TYPE) == mw->type))
			&& (mw->net==NET_BC || (msg->getInt(i , MSG_NET) == mw->net) || (!mw->net && type&MT_MASK_NOTONLIST))
			&& (!mw->uid||uid == mw->uid || (!*mw->uid && type&MT_MASK_NOTONLIST))
			&& ((flag & mw->wflag) == mw->wflag)
			&& (!(flag & mw->woflag))
			&& (mw->id==-1||mw->id==0||msg->getInt(i,MSG_ID)==mw->id)
			;

		/*  IMLOG("IsThat?[%d] i=%d , %d==%d , %d==%d , %s==%s , 0x%x==0x%x , !0x%x flag=0x%x",
		r , i , mw->type,msg->getInt(i , MSG_TYPE)
		, mw->net , msg->getInt(i , MSG_NET)
		, mw->uid , uid
		, mw->wflag , flag & mw->wflag , flag & mw->woflag , flag);
		*/        
		if (r) {
			if (mw->s_size == sizeof(sMESSAGESELECT) && position < mw->position)
				r = false;
			position ++;
		}
		return r;
	}


	void runMessageQueue(sMESSAGESELECT * ms, bool notifyOnly) {
		if (!ms) return;
		oTableImpl msg(tableMessages);
		TableLocker lock(msg, allRows);
		msg->lateSave(false);
		IMLOG("*MessageQueue - inQ=%d , reqNet=%d , reqType=%d" , msg->getRowCount() , ms->net , ms->type);
		//  stack <pair <int , string> > notify;
		unsigned int siz = msg->getRowCount();
		map <int , bool> notify_blank;
		unsigned int i=0;
		unsigned int count = 0;
		cMessage * m = 0;
		while (i < msg->getRowCount()) {
			if (m) messageFree(m, false);
			m = 0;
			int id = msg->getRowId(i);
			int r;
			if (msg->getInt(id , MSG_FLAG) & MF_PROCESSING) {i++;continue;}
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

			msg->setInt(id , MSG_FLAG , m->flag | MF_PROCESSING);
			r = IM_MSG_ok;
			if (!(m->flag & MF_OPENED) && !notifyOnly) {
				if (m->flag & MF_SEND) { // Wysylamy
					r = plugins[msg->getInt(id , MSG_HANDLER)].IMessageDirect(IM_MSG_SEND,(int)m, 0);
				} else {
					// sprawdzanie listy
					//      IMLOG("CHECK %d %s = %d" , m->net , m->fromUid , CFindContact(m->net , m->fromUid));
					if (!(m->type & MT_MASK_NOTONLIST) && *m->fromUid && Contacts::findContact(m->net , m->fromUid)<0)
						r=IM_MSG_delete;  // Jezeli jest spoza listy powinna zostac usunieta
					//          IMessageDirect(Plug[0] , IMI_MSG_NOTINLIST , (int)m);
					else
						r = plugins[msg->getInt(id , MSG_HANDLER)].IMessageDirect(IM_MSG_OPEN,(int)m, 0);
				}
			} // MF_OPENED
			if (r & IM_MSG_delete) {
				IMLOG("_Wiadomosc obsluzona - %d r=%x",msg->getInt(id , MSG_ID) , r);
				//      string test = m->fromUid;
				//      test = notify.top().second;
				//      test = notify.top().second.c_str();
				//      notify.push(make_pair(m->net , m->fromUid));
				if (cntID != -1) {SETCNTI(cntID , CNT_NOTIFY , NOTIFY_AUTO);}
				msg->removeRow(id);
				continue;
			}
			else if (r & IM_MSG_update) {
				updateMessage(id , m);
			}
			if (!(r & IM_MSG_processing)) {
				m->flag&=~MF_PROCESSING;
				msg->setInt(id , MSG_FLAG , m->flag);
			}
			if (cntID != -1 && m->notify) {
				setCntInt(cntID , CNT_NOTIFY , m->notify);
				SETCNTI(cntID , CNT_NOTIFY_MSG , m->id);
				SETCNTI(cntID , CNT_ACT_PARENT , m->action.parent);
				SETCNTI(cntID , CNT_ACT_ID , m->action.id);
			}
			i++;
		}
		if (m) messageFree(m, false);
		m = 0;
		ICMessage(IMI_NOTIFY,NOTIFY_AUTO);
		//notify.clear();
		if (siz != msg->getRowCount()) msg->lateSave(true);
		return;
	}

	int messageNotify(sMESSAGENOTIFY * mn) {
		oTableImpl msg(tableMessages);
		int i = msg->getRowCount()-1;
		mn->action = sUIAction(0,0);
		mn->notify = 0;
		mn->id = 0;
		int found=0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == NET %d UID %s" , mn->net , mn->uid , msg->getInt(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
			if ((msg->getInt(i , MSG_NET) == mn->net && msg->getString(i , MSG_FROMUID) == mn->uid)
				|| (!mn->net && !*mn->uid && msg->getInt(i,MSG_TYPE)&MT_MASK_NOTONLIST)
				)
			{
				mn->action.parent = msg->getInt(i , MSG_ACTIONP);
				mn->action.id = msg->getInt(i , MSG_ACTIONI);
				mn->notify = msg->getInt(i , MSG_NOTIFY);
				mn->id = msg->getInt(i , MSG_ID);
				found++;
				if (mn->action.id || mn->notify) return found;
			}
			i--;
		}
		return found;
	}


	int messageWaiting(sMESSAGESELECT * mw) {
		oTableImpl msg(tableMessages);
		int i = msg->getRowCount()-1;
		int found=0;
		unsigned int count = 0;
		//  if (!mw->uid[0]) return 0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == TYPE %d NET %d UID %s" , mw->net , mw->uid ,  msg->getInt(i , MSG_TYPE) , msg->getInt(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
			//    int flag = msg->getInt(i , MSG_FLAG);
			if (isThatMessage(i , mw , count))
			{
				found++;
			}
			i--;
		}
		return found;
	}

	int getMessage(sMESSAGEPOP * mp , cMessage * m) {
		oTableImpl msg(tableMessages);
		int i = msg->getRowCount()-1;
		unsigned int count = 0;
		//  if (!mw->uid[0]) return 0;
		while (i>=0) {
			//    IMLOG("net %d uid %s == TYPE %d NET %d UID %s" , mw->net , mw->uid ,  msg->getInt(i , MSG_TYPE) , msg->getInt(i , MSG_NET) , Msg.getch(i , MSG_FROMUID));
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
		oTableImpl msg(tableMessages);
		unsigned int c = 0;
		int i = msg->getRowCount()-1;
		unsigned int count = 0;
		while (i>=0 && c<limit) {
			/*          if (msg->getInt(i,MSG_TYPE)&MT_MASK_NOTONLIST)
			notify.push_back(make_pair(0 , ""));
			else
			notify.push_back(make_pair(msg->getInt(i,MSG_NET) , Msg.getch(i,MSG_FROMUID)));
			*/
			if (isThatMessage(i , mp , count))
			{
				int cntID = msg->getInt(i,MSG_TYPE)&MT_MASK_NOTONLIST ? 0: ICMessage(IMC_FINDCONTACT ,msg->getInt(i,MSG_NET) , msg->getInt(i,MSG_FROMUID));
				if (cntID != -1) SETCNTI(cntID , CNT_NOTIFY , NOTIFY_AUTO);
				//if (!(msg->getInt(i,MSG_FLAG)&MF_SEND))
				msg->removeRow(i);
				c++;
				//        return 1;
			}
			i--;
		}
		/*  for (vector <pair <int , string> >::iterator notify_it = notify.begin() ; notify_it!=notify.end(); notify_it++) {
		IMessageDirect(Plug[0],IMI_NOTIFY,IMessage(IMC_FINDCONTACT,0,0,notify_it->first,(int)(notify_it->second.c_str())));
		}
		*/
		ICMessage(IMI_NOTIFY,NOTIFY_AUTO);

		return c;

	}


	void messageProcessed(int id , bool remove) {
		oTableImpl msg(tableMessages);
		id = DataTable::flagId(id);
		msg->setInt(id , MSG_FLAG , msg->getInt(id , MSG_FLAG) & (~MF_PROCESSING));
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
		oTableImpl msg(tableMessages);
		TableLocker lock(msg);
		while (i < msg->getRowCount()) {
			if (newMessage(&makeMessage(i, false),1,i)) i++;
		}
	}

};};