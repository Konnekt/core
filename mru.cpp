#include "stdafx.h"
#include "konnekt_sdk.h"
#include "mru.h"
#include "profiles.h"
#include "tables.h"
#include "threads.h"

#define MRU_FLAG  (tColId)0
#define MRU_ID    1
#define MRU_VALUE 2

#define MRU_SEPCHAR char(1)

using namespace Tables;
using namespace Stamina;

namespace Konnekt { namespace MRU {

	tTableId tableMRU;

	void init() {
		oTable mru = registerTable(Ctrl, "MRU", optAutoLoad | optAutoSave | optAutoUnload | optDiscardLoadedColumns | optMakeBackups | optUseCurrentPassword);
		mru->setFilename("mru.dtb");
		mru->setDirectory();

		tableMRU = mru->getTableId();

		mru->setColumn(MRU_FLAG, ctypeInt);
		mru->setColumn(MRU_ID, ctypeString | cflagXor);
		mru->setColumn(MRU_VALUE, ctypeString | cflagXor);

	}


	// Porzadkuje liste
	bool update(sMRU * mru) {
		if (!mru || !mru->name || !mru->name[0] || !mru->count
			|| !mru->current || !mru->current[0]) return false;
		// Znajdujemy koniec listy

		int i = 0;
		int count = 0;
		int found = -1;
		while (count<mru->count && mru->values[count] && *mru->values[count]) {
			if (!strcmp(mru->current , mru->values[count])) found = count;
			count ++;
		}
		mru->removed = 0;
		if (count < mru->count) {
			if (found == -1) found = count;
		} else
			if (found == -1) {
				found = count - 1;
			}
			mru->removed = mru->values[found];
			// Przesuwamy ca³¹ listê
			while (found) {
				mru->values[found] = mru->values[found-1];
				found --;
			}
			mru->values[0]=(char*)mru->current;
			return true;
	}

	int get(sMRU * mru) {
		if (!mru || !mru->name || !mru->name[0] || !mru->count
			/*|| !mru->buffSize || !mru->values || !mru->values[0]*/) return 0;
			if (mru->flags & MRU_GET_USETEMP) mru->flags |= MRU_GET_ONEBUFF;
		char * temp = mru->flags & MRU_GET_USETEMP ? (char*)TLSU().buffer().getBuffer(2048) : mru->buffer;
		size_t tempSize = mru->flags & MRU_GET_USETEMP ? 2048 : mru->buffSize;
		if (!temp || tempSize < (size_t)(4*mru->count + mru->count)) return 0;
		if (mru->flags & MRU_GET_ONEBUFF) {
			// Musimy wstawiæ puste wskaŸniki do bufora.
			mru->values = (char**)temp;
			memset(temp , 0 , 4*mru->count);
			temp += 4*mru->count;
			tempSize -= 4*mru->count;
			mru->values[0] = temp;
		} else {
			tempSize = mru->buffSize;
		}

		oTableImpl dt(tableMRU);
		dt->load();

		tRowId row = dt->findRow(0, DT::Find::EqStr(dt->getColumn(MRU_ID), mru->name)).getId();
		// Zerujemy wszystkie bufory.
		int i = 0;
		while (i<mru->count && mru->values[i]) {
			mru->values[i][0] = 0;
			i++;
		}
		if (row == DT::rowNotFound) return 0; // Udalo sie znalezc NIC, ale udalo :]
		string entry = dt->getString(row , MRU_VALUE);
		if (entry.empty()) return 0;
		if (*(entry.end()-1) == MRU_SEPCHAR) entry.erase(entry.end()-1);
		size_t current = 0;
		size_t next = 0;
		i = 0;
		int j = 0; // Prawdziwych

		while (current != -1 && mru->values[j] && i<mru->count && tempSize) {
			next = entry.find(MRU_SEPCHAR , current);

			strncpy(mru->values[j] , entry.substr(current , next==-1?-1:next-current).c_str() , tempSize);
			mru->values[j][tempSize-1] = 0;
			if (*mru->values[j]) {
				j++;
				if (mru->flags & MRU_GET_ONEBUFF) {
					size_t len = strlen(mru->values[j-1]);
					tempSize -= len+1 <= tempSize ? len+1 : tempSize;
					if (tempSize) 
						temp += len+1;
					else 
						temp = 0;
					if (j < mru->count) 
						mru->values[j] = temp;
				}
			}
			current = next==-1?-1:next+1;
			i++;
		}
		return j;
	}

	bool set(sMRU * mru) {
		if (!mru || !mru->name || !mru->name[0]) return 0;
		if (mru->flags & MRU_SET_LOADFIRST) {
			MRU::get(mru);
		}					   
		// Update musi odbywaæ siê na kopii!
		sMRU MRU2 = *mru;
		int i = 0;
		MRU2.values = new char*[mru->count];
		for (i = 0; i<mru->count; i++)
			MRU2.values[i] = mru->values[i];
		MRU::update(&MRU2);
		string entry = "";
		i = 0;
		while (i<MRU2.count && MRU2.values[i]) {
			if (*MRU2.values[i]) {
				entry += MRU2.values[i];
				entry += MRU_SEPCHAR;
			}
			i++;
		}

		{
			oTableImpl dt(tableMRU);
			TableLocker lock(dt);
			dt->load();

			tRowId row = dt->findRow(0, DT::Find::EqStr(dt->getColumn(MRU_ID), mru->name)).getId();

			if (row==-1) {
				row = dt->addRow();
				dt->setString(row, MRU_ID, mru->name);
			}
			dt->setString(row , MRU_VALUE, entry);
			dt->lateSave(true);
		}
		delete [] MRU2.values;
		return true;
	}
};};