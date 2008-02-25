#include "stdafx.h"
#include "message_queue.h"
#include "contacts.h"
#include "plugins.h"
#include "imessage.h"
#include "threads.h"

namespace Konnekt { namespace Messages {
  SharedPtr<MessageQueue> MessageQueue::_inst = 0;

  void MessageQueue::init() {
    msg = tableMessages;
  }

  void MessageQueue::deinit() {
    msg.reset();
  }

  void MessageQueue::loadMessages() {
    unsigned int i = 0;

    TableLocker lock(msg);
    while (i < msg->getRowCount()) {
      Message m;
      readMessage(i, &m);
      if (addMessage(&m, true, i)) i++;
    }
  }

  void MessageQueue::clearMessages() {
    unsigned int i = 0;

    TableLocker lock(msg);

    tRowId row = msg->getRowCount() -1;
    while (row > -1) {
      msg->removeRow(row);
      row--;
    }
  }

  int MessageQueue::addMessage(Message* m, bool load, tRowId row) {
    bool candelete = true;
    bool notinlist = false;

    msg->lateSave(false);

    m->setOneFlag(Message::flagLoaded, load);

    // TODO przekierowac do UI
    if (!(m->getFlags() & Message::flagSend) && !(m->getType() & Message::typeMask_NotOnList) && 
      m->getFromUid().size() && Contacts::findContact(m->getNet(), (char*) m->getFromUid().a_str()) < 0) {

        if (!ICMessage(IMI_MSG_NOTINLIST, (int) m)) {
          candelete = true;
          notinlist = true;
          goto messagedelete;
        }
    }

    if (!load) {
      row = msg->getRowPos(msg->addRow());
    }

    m->setId(msg->getRowId(row));

    int i = 0;
    int r = 0;

    iMessageHandler::enMessageQueue rcvtype = m->getFlags() & Message::flagSend 
      ? iMessageHandler::mqReceiveSend : iMessageHandler::mqReceiveOpen;

    while (i < mhlist.count()) {
      MessageHandlerList::Handler& handler = mhlist[i];

      if (handler()->handlingMessage(rcvtype, m)) {

        r = handler()->handleMessage(m, rcvtype, handler.getPriority());

        if (r & Message::resultDelete) {
          candelete = true;
          break;
        } else if (r & Message::resultOk) {
          candelete = false;
        }
      }
      i++;
    }

/*    // Core bez UI nie zadziala, zatem UI musi istniec
    if (m->getFlags() & Message::flagHandledByUI) {

      id = 0;
      while (id < mhlist.count()) {
        MessageHandlerList::Handler& h = mhlist[id];
        if (h.getPlugin()->getPluginId() == pluginUI) {
          handler = h.getId();
        }
        id++;
      }
    }
    */

/*

    r = plugins[pluginUI].IMessageDirect(Message::IM::imReceiveMessage, (int)m, 0);
    if (r & Message::resultDelete) {
      handler = -1;
      goto messagedelete;
    }
    if (r & Message::resultOk) handler = Konnekt::pluginUI;

    for (Plugins::tList::reverse_iterator it = plugins.rbegin(); it != plugins.rend(); ++it) {
      Plugin& plugin = **it;
      if (plugin.getId() == pluginUI) break;

      m->setId(0);

      if (!(plugin.getType() & IMT_ALLMESSAGES) && (!(plugin.getType() & IMT_MESSAGE) || 
        (m->getNet() && plugin.getNet() && plugin.getNet() != m->getNet()))) {
          continue;
      }

      r = plugin.IMessageDirect(Message::IM::imReceiveMessage,(int)m, 0);
      if (r & Message::resultDelete) { // Wiadomosc nie zostaje dodana
        handler = -1;
        goto messagedelete;
      }
      if (r & Message::resultOk) {
        handler = plugins.getIndex(plugin.getId());
      }
    }

    if (m->getFlags() & Message::flagHandledByUI) handler = Konnekt::pluginUI;
*/
messagedelete:
    msg->lateSave(false);

    if (candelete) {
      // Zapisywanie w historii
      if (m->getType() == Message::typeMessage && !(m->getFlags() & Message::flagDontAddToHistory)) {
        sHISTORYADD ha;
        ha.m = m;
        ha.dir = "deleted";
        ha.cnt = 0;
        ha.name = m->getFlags() & Message::flagSend ? "nie wys³ane" : (notinlist ? "spoza listy" : "nie obs³u¿one");
        ha.session = 0;
        plugins[pluginUI].IMessageDirect(IMI_HISTORY_ADD, (int)&ha, 0);
      }

      IMLOG("_Wiadomosc bez obslugi lub usunieta - %.50s...", m->getBody().a_str());

      if (!notinlist || load) msg->removeRow(row);

      m->setId(0);

    } else {
      int cntID = m->getType() & Message::typeMask_NotOnList ? 0 
        : ICMessage(IMC_FINDCONTACT, m->getNet(),(int) m->getFromUid().a_str());

      if (cntID != -1) cnt->setInt(cntID, CNT_LASTMSG, m->getId());

      IMLOG("- Wiadomoœæ %d %s", m->getId(), load ? "jest w kolejce" : "dodana do kolejki");

      m->setOneFlag(Message::flagLoaded, false);

      {
        TableLocker lock(msg, row);
        updateMessage(row, m);
        msg->setInt(row, Message::colId, m->getId());
      }

      // stats
      if (!load && m->getType() == Message::typeMessage) {
        if (m->getFlags() & Message::flagSend) {
          _msgSent++;
        } else {
          _msgRecv++;
        }
      }

      msg->lateSave(true);
    }

    return m->getId();
  }

  int MessageQueue::removeMessage(MessageSelect* ms, unsigned int limit) {
    TableLocker lock(msg);

    if (!limit) limit = 1;

    unsigned int c = 0;

    DT::tRowId row = (DT::tRowId) msg->getRowCount() - 1;

    while (findMessage(ms, row, !c) && limit > c) {
      int cntID = msg->getInt(row, Message::colType) & Message::typeMask_NotOnList
          ? 0 : ICMessage(IMC_FINDCONTACT, msg->getInt(row, Message::colNet), 
          (int)msg->getString(row, Message::colFromUid).a_str());

      if (cntID != -1) SETCNTI(cntID, CNT_NOTIFY, NOTIFY_AUTO);
      msg->removeRow(row);

      c++;
      row--;
    }

    ICMessage(IMI_NOTIFY, NOTIFY_AUTO);
    return c;
  }

  void MessageQueue::readMessage(tRowId row, Message* m) {
    TableLocker lock(msg, row);

    m->setId(msg->getInt(row, Message::colId));
    m->setNet((Net::tNet) msg->getInt(row, Message::colNet));
    m->setType((Message::enType) msg->getInt(row, Message::colType));

    m->setFromUid(msg->getString(row, Message::colFromUid));
    m->setToUid(msg->getString(row, Message::colToUid));
    m->setBody(msg->getString(row, Message::colBody));
    m->setExt(msg->getString(row, Message::colExt));

    m->setFlags((Message::enFlags)msg->getInt(row, Message::colFlag));

    m->setAction(sUIAction(msg->getInt(row, Message::colActionP), msg->getInt(row, Message::colActionI)));
    m->setNotify(msg->getInt(row, Message::colNotify));
    m->setTime(msg->getInt64(row, Message::colTime));
  }

  void MessageQueue::updateMessage(tRowId row, Message* m) {
    TableLocker lock(msg, row);

    msg->setInt(row, Message::colNet, m->getNet());
    msg->setInt(row, Message::colType, m->getType());
    msg->setString(row, Message::colFromUid, m->getFromUid());
    msg->setString(row, Message::colToUid, m->getToUid());
    msg->setString(row, Message::colBody, m->getBody());
    msg->setString(row, Message::colExt, m->getExt());
    msg->setInt(row, Message::colFlag, m->getFlags());
    msg->setInt(row, Message::colActionP, m->getAction().parent);
    msg->setInt(row, Message::colActionI, m->getAction().id);
    msg->setInt(row, Message::colNotify, m->getNotify());
    msg->setInt64(row, Message::colTime, m->getTime());
  }

  bool MessageQueue::getMessage(MessageSelect* ms, Message* m) {
    TableLocker lock(msg);

    DT::tRowId row = (DT::tRowId) msg->getRowCount() - 1;

    if (findMessage(ms, row)) {
      readMessage(row, m);
      return true;
    }
    return false;
  }

  bool MessageQueue::findMessage(MessageSelect* ms, DT::tRowId& row, bool readposition, bool downto) {
    bool checkSize = ms->structSize() == sizeof(MessageSelect) || 
      ms->structSize() != MessageSelect::sizeV1;

    TableLocker lock(msg);
    string uid;

    int found = 0;

    while ((downto && row != DT::rowNotFound) || (!downto && row < msg->getRowCount())) {

      int flag = msg->getInt(row, Message::colFlag);
      int type = msg->getInt(row, Message::colType);

      if (flag & Message::flagSend) {
        uid = msg->getString(row, Message::colToUid);
      } else {
        uid = msg->getString(row, Message::colFromUid);
      }

      bool foundMessage = false;
      foundMessage = ms->type == Message::typeNone || 
        ms->type == Message::typeAll || type == ms->type;
      foundMessage = foundMessage && (ms->net == Net::all || 
        msg->getInt(row, Message::colNet) == ms->net || 
        (ms->net == Net::none && (type & Message::typeMask_NotOnList)));
      foundMessage = foundMessage && (!ms->_chUid || ms->getUid() == uid || 
        (!ms->gotUid() && (type & Message::typeMask_NotOnList)));
      foundMessage = foundMessage && ((flag & ms->wflag) == ms->wflag && 
        !(flag & ms->woflag));
      foundMessage = foundMessage && (ms->id == -1 || ms->id == 0 || 
        msg->getInt(row, Message::colId) == ms->id);

      if (foundMessage) {
        if (readposition) {
          if (!checkSize || found == ms->position) return true;
        } else {
          return true;
        }

        found++;
      }

      row -= downto ? 1 : -1;
    }

    return false;
  }

  int MessageQueue::messageNotify(MessageNotify* mn) {
    TableLocker lock(msg);

    DT::tRowId row = (DT::tRowId) msg->getRowCount() - 1;

    mn->action = sUIAction(0, 0);
    mn->notify = 0;
    mn->id = 0;

    int found = 0;

    while (row != DT::rowNotFound) {

      if ((msg->getInt(row, Message::colNet) == mn->net && 
         mn->getUid() == msg->getString(row, Message::colFromUid)) || 
        (mn->net == Net::none && !mn->getUid().size() && 
         msg->getInt(row, Message::colType) & Message::typeMask_NotOnList)) {

        mn->action.parent = msg->getInt(row , Message::colActionP);
        mn->action.id = msg->getInt(row , Message::colActionI);
        mn->notify = msg->getInt(row , Message::colNotify);
        mn->id = msg->getInt(row , Message::colId);

        found++;

        if (mn->action.id || mn->notify) return found;
      }
      row--;
    }

    return found;
  }

  int MessageQueue::messageWaiting(MessageSelect* ms) {
    TableLocker lock(msg);

    int found = 0;

    DT::tRowId row = (DT::tRowId) msg->getRowCount() - 1;

    while (findMessage(ms, row, !found)) {
      found++;

      row--;
    }

    return found;
  }

  void MessageQueue::messageProcessed(DT::tRowId row, bool remove) {
    TableLocker lock(msg, row);

    msg->setInt(row, Message::colFlag, msg->getInt(row, Message::colFlag) & ~Message::flagProcessing);

    if (remove) {
      MessageSelect ms;
      ms.id = msg->getRowId(row);
      removeMessage(&ms, 1);
    }
  }

  void MessageQueue::runMessageQueue(MessageSelect* ms, bool notifyOnly) {
    TableLocker lock(msg);
    msg->lateSave(false);

    IMLOG("*MessageQueue - inQ=%d , reqNet=%d , reqType=%d" , msg->getRowCount(), ms->net, ms->type);

    DT::tRowId row = 0;
    unsigned int size = msg->getRowCount();

    int found = 0;
    int r = 0;

    Message m;

    while (findMessage(ms, row, false, false)) {
      if (msg->getInt(row, Message::colFlag) & Message::flagProcessing) {
        row--;
        continue;
      }
      found++;

      if (found < (int) ms->position) {
        row--;
        continue;
      }

      readMessage(row, &m);

      int cntID = ((m.getType() & Message::typeMask_NotOnList) || 
        !m.getUid().size()) ? 0 : Contacts::findContact(m.getNet(), (char*) m.getUid().a_str());

      if ((ms->net != Net::all && ms->type > 0) 
        ? (ms->net != m.getNet() && ms->type != m.getType()) 
        : (m.getFlags() & Message::flagRequestOpen && ms->id != m.getId())) {

        row--;
        continue;
      }

      msg->setInt(row, Message::colFlag, m.getFlags() | Message::flagProcessing);

      r = Message::resultOk;
      iMessageHandler::enMessageQueue handlerType = (iMessageHandler::enMessageQueue) 0;

      if (!(m.getFlags() & Message::flagOpened) && !notifyOnly) {
        if (m.getFlags() & Message::flagSend) {
          handlerType = iMessageHandler::mqSend;
        } else {
          if (!(m.getType() & Message::typeMask_NotOnList) && 
            m.getUid().size() && Contacts::findContact(m.getNet(), (char*) m.getFromUid().a_str()) < 0) {
            r = Message::resultDelete;
          } else {
            handlerType = iMessageHandler::mqOpen;
          }
        }
      }

      if (handlerType) {
        int id = 0;

        while (id < mhlist.count()) {
          MessageHandlerList::Handler& handler = mhlist[id];

          if (handler()->handlingMessage(handlerType, &m)) {

            r = handler()->handleMessage(&m, handlerType, handler.getPriority());
            if (r & Message::resultDelete) {
              break;
            } else if (r & Message::resultUpdate) {
              updateMessage(row, &m);
            }
            if (r & Message::resultProcessing) {
              m.setOneFlag(Message::flagProcessing, true);
              break;
            }
          }
          id++;
        }
      }

      if (r & Message::resultDelete) {
        IMLOG("_Wiadomosc obsluzona - %d r=%x", msg->getInt(row, Message::colId), r);

        if (cntID != -1) {
          SETCNTI(cntID, CNT_NOTIFY, NOTIFY_AUTO);
        }
        msg->removeRow(row);
        continue;
      }

      msg->setInt(row, Message::colFlag, m.getFlags());

      if (cntID != -1 && m.getNotify()) {
        SETCNTI(cntID, CNT_NOTIFY, m.getNotify());
        SETCNTI(cntID, CNT_NOTIFY_MSG, m.getId());
        SETCNTI(cntID, CNT_ACT_PARENT, m.getAction().parent);
        SETCNTI(cntID, CNT_ACT_ID, m.getAction().id);
      }

      row++;
    }

    ICMessage(IMI_NOTIFY, NOTIFY_AUTO);

    if (size != msg->getRowCount()) msg->lateSave(true);
  }
};};