#pragma once

namespace Konnekt { namespace Contacts {
  void updateContact(tCntId cntId);
  void updateAllContacts();
  tCntId findContact(tNet net, const StringRef& uid);
};};