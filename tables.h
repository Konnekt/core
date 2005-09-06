#pragma once
#ifndef __KONNEKT_TABLES__
#define __KONNEKT_TABLES__


#include <boost\shared_ptr.hpp>
#include <Konnekt\core_assert.h>
#include <Stamina\ObjectImpl.h>
#include <Stamina\Timer.h>
#include <Stamina\MD5.h>
#include <Stamina\CriticalSection.h>

#include "konnekt_sdk.h"
#include "unique.h"

namespace Konnekt { namespace Tables {


	using Stamina::LockerCS;

	//extern oTable msg;
	extern oTable cfg;
	extern oTable cnt;
	//extern CdTable Plg; // Informacje o wtyczkach
	

	int setColumn(sIMessage_setColumn * sc);
	int saveProfile(bool all=true);
	void deinitialize();


/*#pragma warning( push )
#pragma warning(disable: 4250)*/

	class TableImpl: public ::Stamina::SharedObject<iTable> {
	public:
		typedef Stamina::ObjLocker TableLocker;

		const static int lateSaveDelay = 15000; // 15 sek.

		TableImpl(oPlugin owner, tTableId tableId, enTableOptions tableOpts);

		virtual void __stdcall release();

		int __stdcall getRowPos(tRowId rowId);
		int __stdcall getRowId(unsigned int rowPos);
		tRowId __cdecl _findRow(unsigned int startPos, int argCount, ...);


		enColumnFlag __stdcall getColFlags(tColId colId);
		unsigned int __stdcall getRowCount();
		tColId __stdcall getColId(const char * colName);
		const char * __stdcall getColName(tColId colId);
		unsigned int __stdcall getColCount();
		tColId __stdcall getColIdByPos(unsigned int colPos);
		bool __stdcall get(tRowId rowId , tColId colId , Value & value);
		bool __stdcall set(tRowId rowId , tColId colId , Value & value);
		void __stdcall lockData(tRowId rowId , int reserved=0);
		void __stdcall unlockData(tRowId rowId , int reserved=0);
		tColId __stdcall setColumn(oPlugin plugin, tColId colId , tColType type , DataEntry def = 0 , const char * name = 0);

		tColId setColumn(tColId colId , tColType type , int def = 0 , const char * name = 0) {
			return this->setColumn(Ctrl->getPlugin(), colId, type, (DataEntry)def, name);
		}


		tRowId __stdcall addRow(tRowId rowId = rowNotFound);
		bool __stdcall removeRow(tRowId rowId);
		void __stdcall reset();
		void __stdcall resetData();
		void __stdcall unloadData();

		void __stdcall requestColumns(cCtrl * ctrl, unsigned int net = NET_BROADCAST, unsigned int plugType = IMT_ALL) {
			if (!ctrl) ctrl = Ctrl;
			IM::TableIM ti(IM::setColumns, oTable(this));
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}
		void __stdcall dataChanged(cCtrl * ctrl, tRowId rowId, unsigned int net = NET_BROADCAST, unsigned int plugType = IMT_ALL) {
			if (!ctrl) ctrl = Ctrl;
			IM::TableRow ti(IM::dataChanged, oTable(this), rowId);
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}

		bool __stdcall isLoaded() {
			TableLocker(this);
			return _isLoaded();
		}

		enResult __stdcall load(bool force = false, const char * filePath = 0);
		enResult __stdcall save(bool force = false, const char * filePath = 0);
		void __stdcall lateSave(bool enabled);

		bool __stdcall setOpt(enTableOptions option , bool enabled);
		bool __stdcall getOpt(enTableOptions option) {
			TableLocker(this);
			return (this->_opt & option) == (int)option;
		}

		void __stdcall setFilename(const char * filename) {
			TableLocker(this);
			this->_filename = filename ? filename : "";
		}
		const char * __stdcall _getFilename() {
			TableLocker(this);
			return this->_filename.c_str();
		}
		void __stdcall setDirectory(const char * path = 0);
		const char * __stdcall _getDirectory() {
			TableLocker(this);
			return this->_directory.c_str();
		}
		const char * __stdcall _getTableName() {
			TableLocker(this);
			return Unique::getName(Unique::domainTable, this->getTableId()).c_str();
		}
		tTableId __stdcall getTableId() {
			return this->_tableId;
		}
		oPlugin __stdcall getTableOwner() {
			return this->_owner;
		}

		bool operator & (enTableOptions & b) {
			return this->getOpt(b);
		}

		 bool __stdcall unregisterTable();

		 virtual Stamina::DT::DataTable& __stdcall getDT() {
			 return _dt;
		 }

		 virtual void setTablePassword(const char* password) {
			 _dt.setPassword(password);
		 }


		void __stdcall destroy();

		void broadcastEvent(tIMid imId, bool force = false);
		void broadcastRowEvent(tIMid imId, tRowId rowId, bool force = false);

		CStdString getFilepath() {
			if (this->_directory.empty() || this->_filename.empty())
				return "";
			else
				return this->_directory + "\\" + this->_filename;
		}

	private:
		inline bool __stdcall _isLoaded() {
			return this->_loaded && this->_columnsSet;
		}
		void assertLoaded() {
			if (!this->_isLoaded()) {
				if (this->getOpt(optAutoLoad)) {
					this->load();
				} else {
					K_ASSERT_MSG(this->_isLoaded(), "Tablica musi zostaæ za³adowana, zanim bêdzie mog³a byæ u¿yta!");
				}
			}
		}
		void queryLateSave();
		void onLateSaveTimer();

		enTableOptions _opt;
		oPlugin _owner;
		tTableId _tableId;
		DataTable _dt;
		CStdString _directory, _filename;
		bool _loaded;
		bool _columnsSet;
		CStdString _name;
		boost::shared_ptr<Stamina::TimerDynamic> _lateSaveTimer;

	};

//#pragma warning(pop)

	class oTableImpl:public oTable {
	public:
		oTableImpl(const oTable& b) {
			this->set((TableImpl*)b.get());
		}
		oTableImpl(tTableId tableId) {
			this->setById(tableId);
		}
		oTableImpl(const char * tableName) {
			this->setById(getTableId(tableName));
		}
		oTableImpl(TableImpl * obj = 0) {
			this->set(obj);
		}
		void setById(tTableId tableId);

		inline TableImpl * operator -> () const {
			S_ASSERT(this->get() != 0);
			return (TableImpl*)this->get();
		}
		inline TableImpl & operator * () const {
			return *(TableImpl*)this->get();
		}

	};


	class TableList : public map<tTableId, oTableImpl> {
	public:
		const oTableImpl& operator [] (tTableId tableId) {
			LockerCS l(_cs);
			TableList::iterator it = this->find(tableId);
			if (it == this->end()) {
				static Tables::oTableImpl emptyTable;
				return emptyTable;
			} else {
				//t.set(it->second);
				return it->second;
			}
		}
		bool tableExists(tTableId tableId) {
			if (tableId == rowNotFound) return false;
			LockerCS l(_cs);
			return this->find(tableId) != this->end();
		}
		oTable registerTable(oPlugin owner, tTableId tableId, enTableOptions tableOpts);
		bool unregisterTable(tTableId tableId);

		void saveAll(bool force = false);
		void setProfilePassword(const Stamina::MD5Digest& digest);

		void unregisterAll();

	private:

		Stamina::CriticalSection _cs;
	};

	extern TableList tables;

	inline oTable getTable(tTableId tableId) {
		oTable table = tables[tableId];
		return table->getOpt(optPrivate) ? oTable() : table;
	}

	void debugCommand(sIMessage_debugCommand * arg);



};};

//using Konnekt::Tables::Msg;
//using Konnekt::Tables::Cfg;
//using Konnekt::Tables::Cnt;
//using Konnekt::Tables::Plg;
using Konnekt::Tables::tables;

#endif