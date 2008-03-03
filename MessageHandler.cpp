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

  bool OldPluginMessageHandler::handlingMessage(enMessageQueue queue, Message* msg) {
    return (_queue & queue) && (_net == Net::all /*|| _net == Net::none ||*/ || _net == msg->getNet());
  }

  tMsgResult OldPluginMessageHandler::handleMessage(Message* msg, enMessageQueue queue, 
    enPluginPriority priority) {

      using namespace Tables;

      if (iMessageHandler::mqReceive & queue) {
        tMsgResult r = (tMsgResult) plugins[_plugid].IMessageDirect(Message::IM::imReceiveMessage, 
          (int) msg, 0);

        if (r & Message::resultOk) {
          oTable(tableMessages)->setInt(msg->getId(), Message::colHandler, 
           /* msg->getOneFlag(Message::flagHandledByUI) ? pluginUI : */_plugid);
        }
        return r;
      }

      // handler nie obsluguje wiadomosci
      if (oTable(tableMessages)->getInt(msg->getId(), Message::colHandler) != _plugid) {
        return (tMsgResult) 0;
      }

      tIMid id = iMessageHandler::mqOpen == queue ? Message::IM::imOpenMessage : Message::IM::imSendMessage;

      return (tMsgResult) plugins[_plugid].IMessageDirect(id, (int) msg, 0);
  }

};};