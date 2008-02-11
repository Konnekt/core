#pragma once

#include "konnekt_sdk.h"
#include <Stamina/Object.h>
#include <deque>

namespace Konnekt { namespace Messages {
  using std::deque;

  class MessageHandlerList {
  public:
    class Handler {
      friend MessageHandlerList;

    public:
      typedef unsigned int tHId; 

    public:
      Handler(oPlugin& plugin, iMessageHandler* handler, enPluginPriority priority, 
        iMessageHandler::enMessageQueue type) 
        : _plugin(plugin), _handler(handler), _priority(priority), _type(type) { _id = sid++; }

    public:
      Message::enMessageResult operator() (Message* m, 
        iMessageHandler::enMessageQueue type = (iMessageHandler::enMessageQueue) -1) {
        return _handler->handleMessage(m, (iMessageHandler::enMessageQueue)(getType() & type), getPriority());
      }

    public:
      inline tHId getId() {
        return _id;
      }
      inline enPluginPriority getPriority() {
        return _priority;
      }
      inline iMessageHandler::enMessageQueue getType() {
        return _type;
      }
      inline oPlugin getPlugin() {
        return _plugin;
      }

    private:
      tHId _id;
      oPlugin _plugin;
      Stamina::SharedPtr<iMessageHandler> _handler;
      enPluginPriority _priority;
      iMessageHandler::enMessageQueue _type;

      static tHId sid;
    };

  private:
    typedef deque<Handler> tList;

  public:
    bool registerHandler(oPlugin& plugin, iMessageHandler* handler, iMessageHandler::enMessageQueue type,
      enPluginPriority priority);
    bool unregisterHandler(iMessageHandler* handler);

  public:
    inline Handler& getHandler(unsigned int id) {
      if (_list.size() <= id) {
        throw Stamina::ExceptionString("No handler under this id");
      }
      return (*this)[id];
    }

    inline Handler& operator [] (int id) {
      return _list[id];
    }

    inline Handler& operator [] (Handler::tHId id) {
      for (tList::iterator it = _list.begin(); _list.end() != it; it++) {
        if (it->getId() == id) {
          return *it;
        }
      }
      throw Stamina::ExceptionString("No handler under this handler id");
    }

    inline unsigned int count() {
      return _list.size();
    }

  private:
    tList _list;
  };

  extern MessageHandlerList mhlist;

  class OldPluginMessageHandler : public MessageHandler {
  public:
    OldPluginMessageHandler(int plugid) : _plugid(plugid) { }

  public:
    Message::enMessageResult handleMessage(Message* msg, enMessageQueue queue, Konnekt::enPluginPriority priority);

  private:
    int _plugid;
  };
};};