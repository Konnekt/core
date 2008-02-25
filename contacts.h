#pragma once

namespace Konnekt { namespace Contacts {
  void updateContact(tCntId);
  void updateAllContacts();
  tCntId findContact(tNet net, const StringRef&);
};};