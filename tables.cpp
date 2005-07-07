#include "stdafx.h"
#include "tables.h"
#include "profiles.h"
#include "main.h"
#include "threads.h"

#include <Stamina\ThreadInvoke.h>
#include <boost\bind.hpp>

using Stamina::ObjLocker;

namespace Konnekt { namespace Tables {

	TableList tables;


	CdTable Msg;
	CdTable Cfg;
	CdTable Cnt;
	CdTable Plg; // Informacje o wtyczkach

	int setColumn(sIMessage_setColumn * sc) {
		CdTable * DT = (sc->_table==DTCFG)?&Cfg : (sc->_table==DTCNT)?&Cnt : (sc->_table==DTMSG)?&Msg : 0;
		if (!DT) return 0;
		// jest b³¹d gdy kolumna istnieje i nie by³a za³adowana...
		int index = DT->cols.colindex(sc->_id, false);
		if (index>=0 && !(DT->cols.getcoltype(index) & DT_CF_LOADED)) {
			CtrlEx->PlugOut(sc->sender , STR(_sprintf("Kolumna %d w %d jest ju¿ ZAJÊTA!" , sc->_id , sc->_table)) , 0);
			return 0;
		}
		return DT->cols.setcol(sc->_id , sc->_type , (TdEntry)sc->_def , sc->_name);
	}
	void savePrepare(CdtFileBin & FBin) {
		FBin.dflag |= DT_BIN_DFDBID;
		if (md5digest[0])
			FBin.dflag |= DT_BIN_DFMD5;
		else
			FBin.dflag &= ~DT_BIN_DFMD5;
		memcpy(FBin.md5digest , md5digest , 16);
	}

	void saveDTB(CdTable & tab , const CStdString & file) {
		if (!tab.changed) return;
		tab.lock(DT_LOCKALL);
		CdtFileBin FBin;
		savePrepare(FBin);
		FBin.assign(&tab);
		FBin.save((profileDir + file).c_str());
		tab.changed = false;
		tab.unlock(DT_LOCKALL);
		IMLOG("- "+file+" saved");
	}

	void saveCfg() {
		saveDTB(Cfg , "cfg.dtb");
	}
	void savePlg() {
		saveDTB(Plg , "plg.dtb");
	}
	void saveCnt() {
		saveDTB(Cnt , "cnt.dtb");
	}
	void saveMsg() {
		saveDTB(Msg , "msg.dtb");
	}

	int saveProfile(bool all) {
		if (all) saveRegistry();
		if (all) savePlg();
		Tables::tables.saveAll();
		saveCfg();
		saveCnt();
		saveMsg();
		return 0;
	}


	TableImpl::TableImpl(oPlugin owner, tTableId tableId, enTableOptions tableOpts) {
		this->_owner = owner;
		this->_tableId = tableId;
		this->_opt = (enTableOptions) 0;
		this->setOpt(tableOpts, true);
		
		this->_dt.cxor_key = XOR_KEY;
		this->_dt.dbID = 0;
		this->_loaded = false;
		this->_columnsSet = false;
	}

	int __stdcall TableImpl::getRowPos(tRowId rowId) {
		ObjLocker l (this);
		return _dt.getrowpos(rowId);
	}
	int __stdcall TableImpl::getRowId(unsigned int rowPos) {
		ObjLocker l (this);
		return _dt.getrowid(rowPos);
	}

	tColType __stdcall TableImpl::getColType(tColId colId) {
		ObjLocker l(this);
		int i=_dt.cols.colindex(colId, false);
		if (i<0) return ctypeUnknown;
		return (tColType) _dt.cols.getcolflag(i);
	}
	unsigned int __stdcall TableImpl::getRowCount() {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.getrowcount();
	}
	tColId __stdcall TableImpl::getColId(const char * colName) {
		ObjLocker lock(this);
		return (tColId) _dt.cols.getnameid(colName);
	}
	const char * __stdcall TableImpl::getColName(tColId colId) {
		ObjLocker lock(this);
		int i=_dt.cols.colindex(colId, false);
		if (i<0) return 0;
		return _dt.cols.getcolname(i);
	}
	unsigned int __stdcall TableImpl::getColCount() {
		ObjLocker lock(this);
		return _dt.cols.getcolcount();
	}
	tColId __stdcall TableImpl::getColIdByPos(unsigned int colPos) {
		ObjLocker lock(this);
		return (tColId) _dt.cols.getcolid(colPos);
	}
	bool __stdcall TableImpl::get(tRowId rowId , tColId colId , Value & value) {
		ObjLocker lock(this);
		this->assertLoaded();
		if (value.type == DT_CT_PCHAR && !value.vCChar && value.buffSize==0)
			value.vCChar = TLS().buff2;
		return _dt.getValue(rowId , colId , value.dtRef());
	}
	bool __stdcall TableImpl::set(tRowId rowId , tColId colId , Value & value) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.setValue(rowId , colId , value.dtRef());

	}
	unsigned short __stdcall TableImpl::lockData(tRowId rowId , int reserved) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.lock(rowId);
	}
	unsigned short __stdcall TableImpl::unlockData(tRowId rowId , int reserved) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.unlock(rowId);
	}
	tColId __stdcall TableImpl::setColumn(oPlugin plugin, tColId colId , tColType type , int def , const char * name) {
		ObjLocker lock(this);
		K_ASSERT(_dt.getrowcount() == 0); // nie mo¿emy u¿yæ getRowCount bo jest w nim assertLoaded!
		int index = _dt.cols.colindex(colId, false);
		if (index>=0 && !(_dt.cols.getcoltype(index) & DT_CF_LOADED)) {
			CtrlEx->PlugOut(plugin->getPluginId() , STR(_sprintf("Kolumna %d w %s jest ju¿ ZAJÊTA!" , colId , this->_getTableName())) , 0);
			return colNotFound;
		}
		return (tColId) _dt.cols.setcol(colId , type , (TdEntry)def , name);
	}

	tRowId __stdcall TableImpl::addRow(tRowId rowId) {
		tRowId r;
		{
			ObjLocker lock(this);
			this->assertLoaded();
			r = this->getRowId(_dt.addrow(rowId));
		}
		this->broadcastRowEvent(IM::rowAdded, rowId);
		return r;
	}
	bool __stdcall TableImpl::removeRow(tRowId rowId) {
		{
			ObjLocker lock(this);
			this->assertLoaded();
		}
		this->broadcastRowEvent(IM::rowRemoving, rowId);
		bool r;
		{
			Stamina::ObjLocker lock(this);
			this->assertLoaded(); // na wszelki :]
			r = _dt.deleterow(rowId) != 0;
		}
		this->broadcastRowEvent(IM::rowRemoved, rowId);
		return r;
	}
	void __stdcall TableImpl::reset() {
		this->broadcastEvent(IM::resetting);
		{
			Stamina::ObjLocker lock(this);
			this->_loaded = false;
			this->_columnsSet = false;
			this->_dt.clearrows();
			this->_dt.cols.clear();
		}
	}
	void __stdcall TableImpl::resetData() {
		this->broadcastEvent(IM::resetting);
		{
			Stamina::ObjLocker lock(this);
			this->_dt.clearrows();
		}
	}
	void __stdcall TableImpl::unloadData() {
		this->_loaded = false;
		this->_dt.clearrows();
	}



	bool __stdcall TableImpl::load(bool force, const char * filepath) {
		/*TODO: sprawdzanie poprawnosci hasel*/
		if (!force && this->isLoaded()) return false;
		bool success;
		{
			Stamina::ObjLocker lock(this);
			CStdString file = (filepath) ? filepath : this->getFilepath();
			success = true;
			if (!file.empty()) {
				CdtFileBin fb;
				fb.assign(&this->_dt);
				if (this->getOpt(optDiscardLoadedColumns) == false && this->_columnsSet == false) {
					success = fb.loadAll(file) == 0;
				} else {
					success = fb.load(file) == 0;
				}
			}
			this->_columnsSet = true;
			this->_loaded = true;
		}
		this->broadcastEvent(IM::afterLoad);
		return success;
	}
	bool __stdcall TableImpl::save(bool force, const char * filepath) {
		if ((!force && _dt.changed == false)) return false;
		_lateSaveTimer.reset();
		this->broadcastEvent(IM::beforeSave);
		bool success;
		{
			Stamina::ObjLocker lock(this);
			CStdString file = (filepath) ? filepath : this->getFilepath();
			if (file.empty()) { 
				IMDEBUG(DBG_ERROR, "iTable::save() - no path to file!");
				return false;
			}
			CdtFileBin fb;
			fb.assign(&this->_dt);

			if (this->getOpt(optUseCurrentPassword)) {
				fb.dflag |= DT_BIN_DFDBID;
				if (md5digest[0])
					fb.dflag |= DT_BIN_DFMD5;
				else
					fb.dflag &= ~DT_BIN_DFMD5;
				memcpy(fb.md5digest , md5digest , 16);
			}
			success = fb.save(file) == 0;
		}
		return success;
	}
	void __stdcall TableImpl::lateSave(bool enabled) {
		Stamina::ObjLocker lock(this);
		if (enabled == false)
			_lateSaveTimer.reset();
		else
			Stamina::threadInvoke(mainThread, boost::bind(TableImpl::queryLateSave, this), false);

		IMDEBUG(DBG_FUNC, "iTable::lateSave(%d)", enabled);
	}
	void TableImpl::queryLateSave() {
		Stamina::ObjLocker lock(this);
		IMDEBUG(DBG_FUNC, "iTable::lateSave() queried");
		_lateSaveTimer.reset(Stamina::timerTmplCreate(boost::bind(TableImpl::onLateSaveTimer, this)));
		_lateSaveTimer->start(lateSaveDelay);
	}
	void TableImpl::onLateSaveTimer() {
		IMDEBUG(DBG_FUNC, "iTable::lateSave() invoked");
		this->save();
	}


	bool __stdcall TableImpl::setOpt(enTableOptions option , bool enabled) {
		Stamina::ObjLocker lock(this);
		if (getOpt(option) == enabled) return enabled;
		if (enabled) {

			this->_opt = (enTableOptions)(this->_opt | option);
		} else {
			this->_opt = (enTableOptions)(this->_opt & (~option));
		}
		if (option & optGlobalData) {
			this->setDirectory(0);
		}

		return !enabled;
	}

	void __stdcall TableImpl::setDirectory(const char * path) {
		Stamina::ObjLocker lock(this);
		if (path) {
			this->_directory = path;
			if (!this->_directory.empty() && this->_directory[this->_directory.size()-1] == '\\') {
				this->_directory.erase(this->_directory.size()-1, 1);
			}
		} else {
			if (this->getOpt(optGlobalData)) {
				this->_directory = "%KonnektData%";
			} else {
				this->_directory = "%KonnektProfile%";
			}
		}
		this->_directory = ExpandEnvironmentStrings(this->_directory, MAX_PATH);
	}

	void TableImpl::broadcastEvent(tIMid imId, bool force) {
		if (!force && !getOpt(optBroadcastEvents)) return;
		IM::_tableIM im (imId, oTable(this));
		im.net = NET_BROADCAST;
		im.type = IMT_ALL;
		Ctrl->IMessage(&im);
	}
	void TableImpl::broadcastRowEvent(tIMid imId, tRowId rowId, bool force) {
		if (!force && !getOpt(optBroadcastEvents)) return;
		IM::_tableRowIM im (imId, oTable(this), rowId);
		im.net = NET_BROADCAST;
		im.type = IMT_ALL;
		Ctrl->IMessage(&im);
	}

	void __stdcall TableImpl::destroy() {
		if (this->getOpt(optAutoSave)) {
			this->save();
		}
	}
	bool __stdcall TableImpl::unregisterTable() {
		/*TODO: unregister table*/
		return false;
	}


	void TableList::saveAll(bool force) {
		/*TODO: saveAll*/
		for (iterator it = this->begin(); it != this->end(); it++) {
			oTable table = it->second;
			if (table->getOpt(optAutoSave))
				table->save();
		}
	}



	void testFeature(sIMessage_debugCommand * arg) {
#ifdef __TEST
		IMDEBUG(DBG_TEST_TITLE, "- Konnekt::Tables::test {{{");

		tTableId ttId =  (tTableId) Unique::registerName(Unique::domainTable, "TestTable");

		{
			IMDEBUG(DBG_TEST_TITLE, "Registered, global, autoload, test.dtb");
			Tables::oTable t = tables.registerTable(Ctrl->getPlugin(), ttId, optDefaultSet);
			testResult("Registered", true, (bool)t);
			t ^= optBroadcastEvents;
			t |= optAutoLoad;
			t |= optGlobalData;
			t |= optDiscardLoadedColumns;
			t |= optUseCurrentPassword;
			t ^= optUsePassword;
			testResult("optionSet", true, t & optGlobalData);
			testResult("optionUnSet", 0, (int)(t & optUsePassword));
			testResult("optionComplex", 0, (int)(t & optUseCurrentPassword));
			t->setFilename("test_table.dtb");
			IMDEBUG(DBG_TEST, "filepath = %s \\ %s", t->getDirectory().c_str(), t->getFilename().c_str());
			IMDEBUG(DBG_TEST_TITLE, "setColumn");
			testResult("byId", 1, t->setColumn(1, ctypeInt, 1, "col1"));
			testResult("byName", colNotFound, t->setColumn(colByName, ctypeString, "TEST", "col2"), true);
			tColId col = t->getColId("col2");
			testResult("getColId", colNotFound, col, true);
			testResult("getStr (autoLoad)", "BlaBlaBla", t->getStr(0, "col2"));
			if (t->getRowCount()==0) {
				testResult("addRow", 0, t->addRow(), true);
			}
			testResult("setStr", true, t->setStr(0, "col2", "Testunek"));
			testResult("save", true, t->save());
			IMDEBUG(DBG_TEST_TITLE, "unload");
			t->unloadData();
			testResult("getStr (autoLoad)", "Testunek", t->getStr(0, "col2"));
			IMDEBUG(DBG_TEST_TITLE, "loadAll");
			t ^= optDiscardLoadedColumns;
			t->reset();
			t->setColumn(2, ctypeInt, 0);
			t->load();
			testResult("colCount", 3, t->getColCount());
			testResult("colId", "col2", t->getColName(t->getColIdByPos(2)));
			IMDEBUG(DBG_TEST_TITLE, "release");
			t.set(0);
			IMDEBUG(DBG_TEST_TITLE, "set from id");
			t = oTable(ttId);
			testResult(true, (bool)t);
			testResult("getStr", "Testunek", t->getStr(0, "col2"));

			// find...
			tRowId r1 = t->addRow();
			tRowId r2 = t->addRow();
			tRowId r3 = t->addRow();

			t->setInt(r1, "col1", 100);
			t->setInt(r2, "col1", 200);
			t->setInt(r3, "col1", 300);
			t->setStr(r1, "col2", "a");
			t->setStr(r2, "col2", "b");
			t->setStr(r3, "col2", "c");

			IMDEBUG(DBG_TEST_TITLE, "find eqStr 'b'");
			testResult(r2, t->findRow(0, Find::EqStr(t->getColId("col2"), "b")));
			IMDEBUG(DBG_TEST_TITLE, "find neqStr 'b' start r2");
			testResult(r3, t->findRow(r2, Find(Find::neq, t->getColId("col2"), ValueStr("b"))));

			IMDEBUG(DBG_TEST_TITLE, "find neqStr 'a' AND col1 >= 100");
			testResult(r2, t->findRow(0, Find(Find::neq, t->getColId("col2"), ValueStr("a")), Find(Find::gteq, t->getColId("col1"), ValueInt(100))));

			IMDEBUG(DBG_TEST_TITLE, "find eqStr 'a' AND col1 > 100");
			testResult(rowNotFound, t->findRow(0, Find::EqStr(t->getColId("col2"), "a"), Find(Find::gt, t->getColId("col1"), ValueInt(100))));

			t->removeRow(r1);
			t->removeRow(r2);
			t->removeRow(r3);

			t.set(0);



			{
				Tables::oTable t2 = tables.registerTable(Ctrl->getPlugin(), tableNotFound, optDefaultSet);
				testResult("unregistered", true, (bool)t2);
				t2 ^= optBroadcastEvents;
				t2 |= optAutoSave;
				t2 |= optGlobalData;
				t2->setFilename("test_table.dtb");
				testResult("load", true, t2->load());
				testResult("setStr", true, t2->setStr(0, "col2", "BlaBlaBla"));
			}
		}

		IMDEBUG(DBG_TEST_TITLE, "- Konnekt::Tables::test }}}");
		Unique::unregister(Unique::domainTable, ttId);
#endif
	}


	void debugCommand(sIMessage_debugCommand * arg) {
		CStdString cmd = arg->argv[0];
#ifdef __TEST
		if (cmd == "test") {
			if (arg->argc < 2) {
				IMDEBUG(DBG_COMMAND, " tables");
			} else if (!stricmp(arg->argv[1], "tables")) {
				testFeature(arg);
			}
		}
#endif
	}

};};