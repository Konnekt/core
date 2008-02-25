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
      Handler(iMessageHandler* handler, enPluginPriority priority) 
        : _handler(handler), _priority(priority) { }

    public:
      oMessageHandler& operator() () {
        return getHandler();
      }

    public:
      inline enPluginPriority getPriority() {
        return _priority;
      }
      inline oMessageHandler& getHandler() {
        return _handler;
      }

    private:
      oMessageHandler _handler;
      enPluginPriority _priority;
    };

  private:
    typedef deque<Handler> tList;

  public:
    bool registerHandler(iMessageHandler* handler, enPluginPriority priority);
    bool unregisterHandler(iMessageHandler* handler);

    void clear() {
      _list.clear();
    }

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

    inline unsigned int count() {
      return _list.size();
    }

  private:
    tList _list;
  };

  extern MessageHandlerList mhlist;

  class OldPluginMessageHandler : public MessageHandler {
  public:
    OldPluginMessageHandler(tNet net, int plugid) 
      : MessageHandler((enMessageQueue) -1, net), _plugid(plugid) { }

  public:
    tMsgResult handleMessage(Message* msg, enMessageQueue queue, enPluginPriority priority);
    bool handlingMessage(enMessageQueue type, Message* msg);

  private:
    int _plugid;
  };
};};