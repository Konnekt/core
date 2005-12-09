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
	extern oTable global;
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

		virtual void release();

		int getRowPos(tRowId rowId);
		int getRowId(unsigned int rowPos);
		oRow _findRow(unsigned int startPos, int argCount, ...);


		virtual oRow getRow(tRowId rowId) {
			return _dt.getRow(rowId).get();
		}
		virtual oColumn getColumn(tColId colId) {
			return _dt.getColumn(colId);
		}
		virtual oColumn getColumn(const Stamina::StringRef& colName) {
			return _dt.getColumn(colName);
		}

		virtual oColumn getColumnByPos(unsigned int colPos) {
			return _dt.getColumns().getColumnByIndex(colPos);
		}

		unsigned int getRowCount();
		unsigned int getColCount();
		void lockData(tRowId rowId , int reserved=0);
		void unlockData(tRowId rowId , int reserved=0);
		oColumn setColumn(cCtrl* plugin, tColId colId , tColType type, const StringRef& name = StringRef());
		oColumn setColumn(tColId colId , tColType type, const StringRef& name = StringRef()) {
			return this->setColumn(Ctrl, colId, type, name);
		}

		oRow addRow(tRowId rowId = rowNotFound);
		bool removeRow(tRowId rowId);
		void reset();
		void resetData();
		void unloadData();

		void requestColumns(cCtrl * ctrl, tNet net = Net::broadcast,  enIMessageType plugType = imtAll) {
			if (!ctrl) ctrl = Ctrl;
			IM::TableIM ti(IM::setColumns, oTable(this));
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}
		void dataChanged(cCtrl * ctrl, tRowId rowId, tNet net = Net::broadcast, enIMessageType plugType = imtAll) {
			if (!ctrl) ctrl = Ctrl;
			IM::TableRow ti(IM::dataChanged, oTable(this), rowId);
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}

		bool isLoaded() {
			TableLocker(this);
			return _isLoaded();
		}

		enResult load(bool force = false, const StringRef& filePath = StringRef());
		enResult save(bool force = false, const StringRef& filePath = StringRef());
		void lateSave(bool enabled);

		bool setOpt(enTableOptions option , bool enabled);
		bool getOpt(enTableOptions option) {
			TableLocker(this);
			return (this->_opt & option) == (int)option;
		}

		void setFilename(const StringRef& filename = StringRef()) {
			TableLocker(this);
			this->_filename = filename;
		}
		String getFilename() {
			TableLocker(this);
			return this->_filename;
		}
		void setDirectory(const StringRef& path = StringRef());
		String getDirectory() {
			TableLocker(this);
			return this->_directory;
		}
		tTableId getTableId() {
			return this->_tableId;
		}
		oPlugin getTableOwner() {
			return this->_owner;
		}

		bool operator & (enTableOptions & b) {
			return this->getOpt(b);
		}

		 bool unregisterTable();

		 virtual Stamina::DT::DataTable& getDT() {
			 return _dt;
		 }

		 virtual void setTablePassword(const StringRef& password) {
			 _dt.setPassword(password);
		 }


		void destroy();

		void broadcastEvent(tIMid imId, bool force = false);
		void broadcastRowEvent(tIMid imId, tRowId rowId, bool force = false);

		String getFilepath() {
			if (this->_directory.empty() || this->_filename.empty())
				return "";
			else
				return this->_directory + "\\" + this->_filename;
		}

	private:
		inline bool _isLoaded() {
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
		String _directory, _filename;
		bool _loaded;
		bool _columnsSet;
		String _name;
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
		oTableImpl(const StringRef& tableName) {
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
		void addPasswordDigest(const Stamina::MD5Digest& digest);

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