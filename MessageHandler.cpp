#include "stdafx.h"
#include "MessageHandler.h"
#include "plugins.h"
#include "tables.h"

namespace Konnekt { namespace Messages {
  MessageHandlerList mhlist;

  bool MessageHandlerList::registerHandler(iMessageHandler* handler, enPluginPriority priority) {
    if (priority == priorityNone) 
      priority = priorityStandard;

    tList::iterator it = _list.begin();

    // wyszukanie odpowiedniego miejsca
    while (it != _list.end()) {
      if (it->_handler == handler) {
        return false;
      } else if (it->_priority < priority) {
        break;
      }
      it++;
    }

    tList::iterator itPos = it;

    // unikniecie dublowania tego samego handlera
    while (it != _list.end()) {
      if (it->_handler == handler) {
        return false;
      }
      it++;
    }

    _list.insert(itPos, Handler(handler, priority));

    return true;
  }

  bool MessageHandlerList::unregisterHandler(iMessageHandler* handler) {
    tList::iterator it = _list.begin();

    // wyszukanie odpowiedniego handlera
    while (it != _list.end()) {
      if (it->_handler == handler) {
        _list.erase(it);
        return true;
      }
    }
    return false;
  }

  bool OldPluginMessageHandler::handlingMessage(iMessageHandler::enMessageQueue type, Message* m) {
    return (_mq & type) && (_net == Net::all || _net == Net::none || _net == m->getNet());
  }

  Message::enMessageResult OldPluginMessageHandler::handleMessage(Message* m, enMessageQueue queue, 
    Konnekt::enPluginPriority priority) {

      using namespace Tables;

      if (iMessageHandler::mqReceive & queue) {
        Message::enMessageResult r = (Message::enMessageResult) plugins[_plugid].IMessageDirect(Message::IM::imReceiveMessage, 
          (int) m, 0);

        if (r & Message::resultOk) {
          oTable(tableMessages)->setInt(m->getId(), Message::colHandler, 
            m->getOneFlag(Message::flagHandledByUI) ? pluginUI : _plugid);
        }
        return r;
      }

      // handler nie obsluguje wiadomosci
      if (oTable(tableMessages)->getInt(m->getId(), Message::colHandler) != _plugid) {
        return (Message::enMessageResult) 0;
      }

      tIMid id = iMessageHandler::mqOpen == queue ? Message::IM::imOpenMessage : Message::IM::imSendMessage;

      return (Message::enMessageResult) plugins[_plugid].IMessageDirect(id, (int) m, 0);
  }

};};