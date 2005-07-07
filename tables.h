#pragma once
#include "konnekt_sdk.h"
#include <boost\shared_ptr.hpp>
#include <Konnekt\core_assert.h>
#include <Stamina\ObjectImpl.h>
#include <Stamina\Timer.h>
#include "unique.h"

namespace Konnekt { namespace Tables {

	extern CdTable Msg;
	extern CdTable Cfg;
	extern CdTable Cnt;
	extern CdTable Plg; // Informacje o wtyczkach

	int setColumn(sIMessage_setColumn * sc);
	void saveDTB(CdTable & tab , const CStdString & file);
	void saveCfg();
	void savePlg();
	void saveCnt();
	void saveMsg();
	int saveProfile(bool all);
	void savePrepare(CdtFileBin & FBin);
	int saveProfile(bool all=true);

/*#pragma warning( push )
#pragma warning(disable: 4250)*/

	class TableImpl: public Stamina::SharedObject<iTable> {
	public:
		typedef Stamina::ObjLocker TableLocker;

		const static int lateSaveDelay = 15000; // 15 sek.

		TableImpl(oPlugin owner, tTableId tableId, enTableOptions tableOpts);

		int __stdcall getRowPos(tRowId rowId);
		int __stdcall getRowId(unsigned int rowPos);
        tRowId __cdecl findRow(unsigned int startPos, int argCount, ...);


		tColType __stdcall getColType(tColId colId);
		unsigned int __stdcall getRowCount();
		tColId __stdcall getColId(const char * colName);
		const char * __stdcall getColName(tColId colId);
		unsigned int __stdcall getColCount();
		tColId __stdcall getColIdByPos(unsigned int colPos);
		bool __stdcall get(tRowId rowId , tColId colId , Value & value);
		bool __stdcall set(tRowId rowId , tColId colId , Value & value);
		unsigned short __stdcall lockData(tRowId rowId , int reserved=0);
		unsigned short __stdcall unlockData(tRowId rowId , int reserved=0);
		tColId __stdcall setColumn(oPlugin plugin, tColId colId , tColType type , int def , const char * name);

		tRowId __stdcall addRow(tRowId rowId = rowNotFound);
		bool __stdcall removeRow(tRowId rowId);
		void __stdcall reset();
		void __stdcall resetData();
		void __stdcall unloadData();

		void __stdcall requestColumns(cCtrl * ctrl, unsigned int net = NET_BROADCAST, unsigned int plugType = IMT_ALL) {
			if (!ctrl) ctrl = Ctrl;
			IM::_tableIM ti(IM::setColumns, oTable(this));
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}
		void __stdcall dataChanged(cCtrl * ctrl, tRowId rowId, unsigned int net = NET_BROADCAST, unsigned int plugType = IMT_ALL) {
			if (!ctrl) ctrl = Ctrl;
			IM::_tableRowIM ti(IM::dataChanged, oTable(this), rowId);
			ti.net = net;
			ti.type = plugType;
			ctrl->IMessage(&ti);
		}

		bool __stdcall isLoaded() {
			TableLocker(this);
			return _isLoaded();
		}

		bool __stdcall load(bool force = false, const char * filePath = 0);
		bool __stdcall save(bool force = false, const char * filePath = 0);
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
		void __stdcall setDirectory(const char * path);
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
		CdTable _dt;
		CStdString _directory, _filename;
		bool _loaded;
		bool _columnsSet;
		CStdString _name;
		boost::shared_ptr<Stamina::TimerDynamic> _lateSaveTimer;

	};

//#pragma warning(pop)

	class TableList : public map<tTableId, oTable> {
	public:
		oTable operator [] (tTableId tableId) {
			TableList::iterator it = this->find(tableId);
			Tables::oTable t;
			if (it == this->end())
				return t;
			else {
				t.set(it->second);
				return t;
			}
		}
		bool tableExists(tTableId tableId) {
			if (tableId == rowNotFound) return false;
			return this->find(tableId) != this->end();
		}
		oTable registerTable(oPlugin owner, tTableId tableId, enTableOptions tableOpts) {
			Tables::oTable t;
			if (tableExists(tableId)) return t;
			t.set(new TableImpl(owner, tableId, tableOpts));
			if (tableId != rowNotFound) {
				this->insert(std::pair<tTableId, oTable>(tableId, t));
			}
			return t;
		}
		void saveAll(bool force = false);
	};

	extern TableList tables;

	inline oTable getTable(tTableId tableId) {
		return tables[tableId];
	}

	void debugCommand(sIMessage_debugCommand * arg);


};};

using Konnekt::Tables::Msg;
using Konnekt::Tables::Cfg;
using Konnekt::Tables::Cnt;
using Konnekt::Tables::Plg;
using Konnekt::Tables::tables;
