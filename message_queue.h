#pragma once

#include "konnekt_sdk.h"
#include <Stamina/DataTable/DT.h>
#include "tables.h"

#include "MessageHandler.h"

namespace Konnekt { namespace Messages {
  using namespace Stamina;
  using namespace Tables;

  class MessageQueue: public SharedObject<iSharedObject> {
  private:
    static SharedPtr<MessageQueue> _inst;

  public:
    static MessageQueue* getInstance() {
      if (!_inst.isValid()) {
        _inst = new MessageQueue;
      }
      return _inst;
    }

  private:
    MessageQueue() : _msgRecv(0), _msgSent(0) {}

  public:
    void init();
    void deinit();

  public:
    void loadMessages();
    void clearMessages();

  public:
    int addMessage(Message* m, bool load, DT::tRowId row);
    int removeMessage(MessageSelect* ms, unsigned int limit);
    void readMessage(DT::tRowId row, Message* m);
    void updateMessage(DT::tRowId row, Message* m);
    bool getMessage(MessageSelect* ms, Message* m);
    bool findMessage(MessageSelect* ms, DT::tRowId& row, bool readpostion = true, bool downto = true);

  public:
    int messageNotify(MessageNotify* mn);
    int messageWaiting(MessageSelect* ms);
    void messageProcessed(DT::tRowId row, bool remove);

  public:
    void runMessageQueue(MessageSelect* ms, bool notifyOnly = false);

  public:
    inline int msgSent() const {
      return _msgSent;
    }
    inline int msgRecv() const {
      return _msgRecv;
    }

  private:
    oTableImpl msg;
    int _msgSent;
    int _msgRecv;
  };

};};