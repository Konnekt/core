#include "stdafx.h"
#include "mru.h"
#include "konnekt_sdk.h"
#include "main.h"
#include "profiles.h"

#define MRU_FLAG  0
#define MRU_ID    1
#define MRU_VALUE 2

#define FILE_MRU "mru.dtb"

#define MRU_SEPCHAR char(1)

namespace Konnekt { namespace MRU {

	CdtColDesc descriptor=CdtColDesc(3
		,MRU_FLAG , DT_CT_INT , 0 , ""
		,MRU_ID   , DT_CT_PCHAR | DT_CF_CXOR , "" , ""
		,MRU_VALUE, DT_CT_PCHAR | DT_CF_CXOR , "" , ""
		);




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
		char * temp = mru->flags&MRU_GET_USETEMP?TLS().buff : mru->buffer;
		size_t tempSize = mru->flags&MRU_GET_USETEMP?MAX_STRING : mru->buffSize;
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
		CdtFileBin FBin;
		CdTable TMru;
		TMru.cols = MRU::descriptor;
		TMru.cxor_key = XOR_KEY;
		FBin.assign(&TMru);
		FBin.load((profileDir + FILE_MRU).c_str());
		int row = TMru.findby((TdEntry)mru->name , MRU_ID);
		// Zerujemy wszystkie bufory.
		int i = 0;
		while (i<mru->count && mru->values[i]) {
			mru->values[i][0] = 0;
			i++;
		}
		if (row == -1) return 0; // Udalo sie znalezc NIC, ale udalo :]
		string entry = (char*)TMru.get(row , MRU_VALUE);
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

		CdtFileBin FBin;
		CdTable TMru;
		TMru.cols = MRU::descriptor;
		TMru.cxor_key = XOR_KEY;
		FBin.assign(&TMru);
		FBin.load((profileDir + FILE_MRU).c_str());
		int row = TMru.findby((TdEntry)mru->name , MRU_ID);
		if (row==-1) {
			row = TMru.addrow();
			TMru.set(row , MRU_ID , (TdEntry)mru->name);
		}
		TMru.set(row , MRU_VALUE , (TdEntry)entry.c_str());
		FBin.save((profileDir + FILE_MRU).c_str());
		delete [] MRU2.values;
		return true;
	}
};};