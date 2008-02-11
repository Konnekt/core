#include "stdafx.h"
#include "MessageHandler.h"
#include "plugins.h"

namespace Konnekt { namespace Messages {
  MessageHandlerList mhlist;
  MessageHandlerList::Handler::tHId  MessageHandlerList::Handler::sid = 0;

  bool MessageHandlerList::registerHandler(oPlugin& plugin, iMessageHandler* handler, iMessageHandler::enMessageQueue type, 
    enPluginPriority priority) {

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

      _list.insert(itPos, Handler(plugin, handler, priority, type));

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

  Message::enMessageResult OldPluginMessageHandler::handleMessage(Message* msg, enMessageQueue queue, Konnekt::enPluginPriority priority) {
    tIMid id;

    if (iMessageHandler::mqOpen == queue) {
      id = Message::IM::imOpenMessage;
    } else if (iMessageHandler::mqSend == queue) {
      id = Message::IM::imSendMessage;
    } else if (iMessageHandler::mqReceive == queue) {
      id = Message::IM::imReceiveMessage;
    }

    return (Message::enMessageResult)plugins[_plugid].IMessageDirect(id, (int) msg, 0);
  }

};};