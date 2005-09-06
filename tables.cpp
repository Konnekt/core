#include "stdafx.h"
#include "tables.h"
#include "profiles.h"
#include "main.h"
#include "threads.h"

#include <Stamina\ThreadInvoke.h>
#include <Stamina\DataTable\FileBin.h>
#include <boost\bind.hpp>

using namespace Stamina;

namespace Konnekt { namespace Tables {

	TableList tables;


/*	CdTable Msg;
	CdTable Cfg;
	CdTable Cnt;
	CdTable Plg; // Informacje o wtyczkach
*/
	oTable cfg;
	oTable cnt;

	void deinitialize() {
		tables.unregisterAll();
	}

	int setColumn(sIMessage_setColumn * sc) {
		oTable dt((tTableId)sc->_table);
		if (!dt) return 0;
		// jest b³¹d gdy kolumna istnieje i nie by³a za³adowana...
		return dt->setColumn((tColId)sc->_id , (tColType)sc->_type , (DataEntry)sc->_def , sc->_name);
	}



	int saveProfile(bool all) {
		if (all) saveRegistry();
		//if (all) tables[tablePlugins].save();
		Tables::tables.saveAll();
		return 0;
	}

	TableImpl::TableImpl(oPlugin owner, tTableId tableId, enTableOptions tableOpts) {
		this->_owner = owner;
		this->_tableId = tableId;
		this->_opt = (enTableOptions) 0;
		this->setOpt(tableOpts, true);
		
		this->_dt.setXor1Key(XOR_KEY);
		this->_loaded = false;
		this->_columnsSet = false;
	}

	void __stdcall TableImpl::release() {
		// Zaraz bêdzie 2, czyli ¿e defacto pozostaje tylko jedna referencja - referencja w spisie tablic
		if (this->getUseCount() == 3) {
			if (this->isLoaded() && this->getOpt(optAutoUnload)) {
				this->unloadData();
			}
		}
		Stamina::SharedObject<iTable>::release();
	}

	int __stdcall TableImpl::getRowPos(tRowId rowId) {
		ObjLocker l (this);
		return _dt.getRowPos(rowId);
	}
	int __stdcall TableImpl::getRowId(unsigned int rowPos) {
		ObjLocker l (this);
		return _dt.getRowId(rowPos);
	}

	tRowId __cdecl TableImpl::_findRow(unsigned int startPos, int argCount, ...) {
		ObjLocker l (this);
		this->assertLoaded();
		va_list list;
		va_start(list, argCount);
		tRowId result = _dt.findRow(startPos, argCount, list);
		va_end(list);
		return result;
	}


	enColumnFlag __stdcall TableImpl::getColFlags(tColId colId) {
		ObjLocker l(this);
		return _dt.getColumns().getColumn(colId).getFlags();
	}
	unsigned int __stdcall TableImpl::getRowCount() {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.getRowCount();
	}
	tColId __stdcall TableImpl::getColId(const char * colName) {
		ObjLocker lock(this);
		return _dt.getColumns().getNameId(colName);
	}
	const char * __stdcall TableImpl::getColName(tColId colId) {
		ObjLocker lock(this);
		return _dt.getColumns().getColumn(colId).getName().c_str();
	}
	unsigned int __stdcall TableImpl::getColCount() {
		ObjLocker lock(this);
		return _dt.getColumns().getColCount();
	}
	tColId __stdcall TableImpl::getColIdByPos(unsigned int colPos) {
		ObjLocker lock(this);
		return _dt.getColumns().getColumnByIndex(colPos).getId();
	}
	bool __stdcall TableImpl::get(tRowId rowId , tColId colId , Value & value) {
		ObjLocker lock(this);
		this->assertLoaded();
		if (value.type == ctypeString && !value.vCChar && value.buffSize==0)
			value.vCChar = TLSU().shortBuffer;
		return _dt.getValue(rowId, colId, value);
	}
	bool __stdcall TableImpl::set(tRowId rowId, tColId colId, Value& value) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.setValue(rowId , colId , value);

	}
	void __stdcall TableImpl::lockData(tRowId rowId , int reserved) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.lock(rowId);
	}
	void __stdcall TableImpl::unlockData(tRowId rowId , int reserved) {
		ObjLocker lock(this);
		this->assertLoaded();
		return _dt.unlock(rowId);
	}

	tColId __stdcall TableImpl::setColumn(oPlugin plugin, tColId colId , tColType type , DataEntry def , const char * name) {
		ObjLocker lock(this);
		K_ASSERT(_dt.getRowCount() == 0); // nie mo¿emy u¿yæ getRowCount bo jest w nim assertLoaded!
		Column col = _dt.getColumns().getColumn(colId);
		if (col.empty() == false && col.hasFlag(cflagIsLoaded) == false) {
			CtrlEx->PlugOut(plugin->getPluginId() , stringf("Kolumna %d w %s jest ju¿ ZAJÊTA!" , colId , this->_getTableName()).c_str() , 0);
			return colNotFound;
		}
		return (tColId) _dt.setColumn(colId , type , (DataEntry)def, name);
	}

	tRowId __stdcall TableImpl::addRow(tRowId rowId) {
		tRowId r;
		{
			ObjLocker lock(this);
			this->assertLoaded();
			r = this->getRowId(_dt.addRow(rowId));
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
			r = _dt.deleteRow(rowId);
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
			this->_dt.clearRows();
			this->_dt.clearColumns();
		}
	}

	void __stdcall TableImpl::resetData() {
		this->broadcastEvent(IM::resetting);
		{
			Stamina::ObjLocker lock(this);
			this->_dt.clearRows();
		}
	}

	void __stdcall TableImpl::unloadData() {
		if (this->getOpt(optAutoSave)) {
			this->save();
		}
		{
			Stamina::ObjLocker lock(this);
			this->_loaded = false;
			this->_dt.clearRows();
		}
	}

	enResult __stdcall TableImpl::load(bool force, const char * filepath) {
		/*TODO: sprawdzanie poprawnosci hasel*/
		if (!force && this->isLoaded()) return errAlreadyLoaded;
		enResult result;
		{
			Stamina::ObjLocker lock(this);
			CStdString file = (filepath) ? filepath : this->getFilepath();
			file = Stamina::expandEnvironmentStrings(file);
			result = errFileNotFound;
			if (!file.empty()) {
				FileBin fb;
				fb.assign(_dt);
				if (this->getOpt(optDiscardLoadedColumns) == false && this->_columnsSet == false) {
					result = fb.loadAll(file);
				} else {
					result = fb.load(file);
				}
			}
			this->_columnsSet = true;
			this->_loaded = true;
		}
		this->broadcastEvent(IM::afterLoad);
		return result;
	}


	enResult __stdcall TableImpl::save(bool force, const char * filepath) {
		if ((!force && _dt.isChanged() == false)) return errNotChanged;
		if (_columnsSet == false || (getOpt(optAutoLoad) && !isLoaded())) return errNotLoaded;
		_lateSaveTimer.reset();
		this->broadcastEvent(IM::beforeSave);
		enResult result;
		{
			Stamina::ObjLocker lock(this);
			CStdString file = (filepath) ? filepath : this->getFilepath();
			file = Stamina::expandEnvironmentStrings(file);
			if (file.empty()) { 
				IMDEBUG(DBG_ERROR, "iTable::save() - no path to file!");
				return errFileNotFound;
			}
			FileBin fb(_dt);

			/*
			if (this->getOpt(optUseCurrentPassword)) {
				fb.dflag |= DT_BIN_DFDBID;
				if (md5digest[0])
					fb.dflag |= DT_BIN_DFMD5;
				else
					fb.dflag &= ~DT_BIN_DFMD5;
				memcpy(fb.md5digest , md5digest , 16);
			}
			*/
			enFileOperation op = noOperation;
			if (getOpt(optUseOldCryptVersion)) {
				op = saveOldCryptVersion;
			}
			if (getOpt(optMakeBackups)) {
				fb.makeBackups = true;
			}
			if (getOpt(optUseTemporary)) {
				fb.useTempFile = true;
			}
			result = fb.save(file, op);
		}
		return result;
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

		if (option & optUseCurrentPassword) {
			this->_dt.setPasswordDigest(passwordDigest);
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
		this->_directory = expandEnvironmentStrings(this->_directory, MAX_PATH);
	}

	void TableImpl::broadcastEvent(tIMid imId, bool force) {
		if (!force && !getOpt(optBroadcastEvents)) return;
		IM::TableIM im (imId, oTable(this));
		im.net = NET_BROADCAST;
		im.type = IMT_ALL;
		Ctrl->IMessage(&im);
	}

	void TableImpl::broadcastRowEvent(tIMid imId, tRowId rowId, bool force) {
		if (!force && !getOpt(optBroadcastEvents)) return;
		IM::TableRow im (imId, oTable(this), rowId);
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
		return tables.unregisterTable(this->getTableId());
	}

	void TableList::saveAll(bool force) {
		LockerCS l(_cs);
		for (iterator it = this->begin(); it != this->end(); it++) {
			oTable table = it->second;
			if (table->getOpt(optAutoSave))
				table->save();
		}
	}

	void TableList::setProfilePassword(const MD5Digest& digest) {
		LockerCS l(_cs);
		for (iterator it = this->begin(); it != this->end(); it++) {
			oTable table = it->second;
			if (table->getOpt(optUseCurrentPassword)) {
				if (table->getOpt(optAutoLoad) && table->isLoaded() == false) {
					table->load();
				}
				if (table->isLoaded() == false) continue;
				table->getDT().setPasswordDigest(digest);
				if (table->getOpt(optAutoSave)) {
					table->save();
				}
			}
		}

	}

	oTable TableList::registerTable(oPlugin owner, tTableId tableId, enTableOptions tableOpts) {
		LockerCS l(_cs);
		Tables::oTable t;
		if (tableExists(tableId)) return t;
		TableImpl* table = new TableImpl(owner, tableId, tableOpts);
		t.set(table);
		if (tableId != tableNotFound) {
			this->insert(std::pair<tTableId, oTable>(tableId, t));
		}
		table->broadcastEvent(IM::tableRegistered);
		return t;
	}


	bool TableList::unregisterTable(tTableId tableId) {
		LockerCS l(_cs);
		if (!tableExists(tableId)) return false;
		((*this)[tableId])->broadcastEvent(IM::tableUnregistering);
		this->erase(tableId);
		return true;
	}

	void TableList::unregisterAll() {
		LockerCS l(_cs);
		while (this->size()) {
			unregisterTable(this->begin()->first);
		}
	}


	void oTableImpl::setById(tTableId tableId) {
		this->set(*tables[tableId]);
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
			testResult("getStr (autoLoad)", "BlaBlaBla", t->getCh(0, "col2"));
			if (t->getRowCount()==0) {
				testResult("addRow", 0, t->addRow(), true);
			}
			testResult("setStr", true, t->setStr(0, "col2", "Testunek"));
			testResult("save", true, t->save());
			IMDEBUG(DBG_TEST_TITLE, "unload");
			t->unloadData();
			testResult("getStr (autoLoad)", "Testunek", t->getCh(0, "col2"));
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
			testResult("getStr", "Testunek", t->getCh(0, "col2"));

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
		Unique::instance()->getDomain(Unique::domainTable)->unregister(ttId);
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