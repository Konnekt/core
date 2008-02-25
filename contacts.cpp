#include "stdafx.h"
#include "main.h"
#include "tables.h"
#include "konnekt_sdk.h"
#include "contacts.h"
#include "pseudo_export.h"

namespace Konnekt {namespace Contacts {
  using namespace Stamina;

  void updateContact(tCntId cntId) {
    if ((cntId = Tables::cnt->getRowPos(cntId)) == cntNotExist) {
      return;
    }

    bool ignored = ICMessage(IMC_IGN_FIND, Tables::cnt->getInt(cntId, Contact::colNet), 
      (int) Tables::cnt->getString(cntId, Contact::colUid).a_str()) == 1;

    tStatus status = Tables::cnt->getInt(cntId, Contact::colStatus) & ~ST_IGNORED;

    if (ignored) {
      status = status | ST_IGNORED;
    }

    Tables::cnt->setInt(cntId, Contact::colStatus, status);
  }


  void updateAllContacts() {
    for (tRowId row = 0; row < Tables::cnt->getRowCount(); row++) {
      updateContact(row);
    }
  }


  tCntId findContact(tNet net, const StringRef& uid) {
    if (net == Net::none && uid.empty()) return 0;

    if (uid.empty()) {
      return cntNotExist;
    }

    if (net == Net::none) {
      tRowId id = Tables::cnt->getRowId(chtoint(uid));
      if (Tables::cnt->getRowPos(id) != Tables::rowNotFound) {
        return (tCntId) id;
      } else {
        return cntNotExist;
      }
    }

    if (net == Net::all) {
      return Tables::cnt->findRow(0, DT::Find::EqStr(Tables::cnt->getColumn(Contact::colDisplay), uid)).getId();
    } else {
      return Tables::cnt->findRow(0, DT::Find::EqInt(Tables::cnt->getColumn(Contact::colNet), net), 
        DT::Find::EqStr(Tables::cnt->getColumn(Contact::colUid), uid)).getId();
    }

    return cntNotExist;
  }
};};