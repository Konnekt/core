#include "stdafx.h"
#include "tables.h"
#include "profiles.h"
#include "main.h"
#include "threads.h"
#include "tables_interface.h"
#include "plugins_ctrl.h"

#include <Stamina\ThreadInvoke.h>
#include <Stamina\DataTable\FileBin.h>
#include <boost\bind.hpp>

using namespace Stamina;

namespace Konnekt { namespace Tables {

	TableList tables;

	StaticObj<Interface_konnekt> iface;
	StaticObj<Interface_konnekt_silent> ifaceSilent;

/*	CdTable Msg;
	CdTable Cfg;
	CdTable Cnt;
	CdTable Plg; // Informacje o wtyczkach
*/
	oTable cfg;
	oTable cnt;
	oTable global;

	void deinitialize() {
		cfg.reset();
		cnt.reset();
		global.reset();
		tables.unregisterAll();
	}

	int setColumn(sIMessage_setColumn * sc) {
		oTable dt((tTableId)sc->_table);
		Plugin* plugin = plugins.get(sc->sender);
		if (!dt || !plugin) return 0;
		// jest b³¹d gdy kolumna istnieje i nie by³a za³adowana...
		oColumn col = dt->setColumn(plugin->getCtrl(), (tColId)sc->_id, (tColType)sc->_type, sc->_name);

		if (sc->_def) {
			switch(sc->_type & ctypeMask) {
				case ctypeInt:
					col->setInt(rowDefault, (int)sc->_def);
					break;
				case ctypeInt64:
					col->setInt64(rowDefault, *(__int64*)sc->_def);
					break;
				case ctypeDouble:
					col->setDouble(rowDefault, *(double*)sc->_def);
					break;
				case ctypeString:
					col->setString(rowDefault, (char*)sc->_def);
					break;
			}
		}

		return col->getId();
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

	void TableImpl::release() {
		// Zaraz bêdzie 2, czyli ¿e defacto pozostaje tylko jedna referencja - referencja w spisie tablic
		if (this->getUseCount() == 3) {
			if (this->isLoaded() && this->getOpt(optAutoUnload)) {
				this->unloadData();
			}
		}
		Stamina::SharedObject<iTable>::release();
	}

	int TableImpl::getRowPos(tRowId rowId) {
		ObjLocker l (this, lockRead);
		return _dt.getRowPos(rowId);
	}
	int TableImpl::getRowId(unsigned int rowPos) {
		ObjLocker l (this, lockRead);
		return _dt.getRowId(rowPos);
	}

	oRow __cdecl TableImpl::_findRow(unsigned int startPos, int argCount, ...) {
		ObjLocker l (this, lockRead);
		this->assertLoaded();
		va_list list;
		va_start(list, argCount);
		oRow result = _dt.findRow(startPos, argCount, list);
		va_end(list);
		return result;
	}


	unsigned int TableImpl::getRowCount() {
		ObjLocker lock(this, lockDefault);
		this->assertLoaded();
		return _dt.getRowCount();
	}
	unsigned int TableImpl::getColCount() {
		ObjLocker lock(this, lockDefault);
		return _dt.getColumns().getColCount();
	}

	void TableImpl::lockData(tRowId rowId , int reserved) {
		ObjLocker lock(this, lockDefault);
		this->assertLoaded();
		return _dt.lock(rowId);
	}

	void TableImpl::unlockData(tRowId rowId , int reserved) {
		ObjLocker lock(this, lockDefault);
		this->assertLoaded();
		return _dt.unlock(rowId);
	}

	oColumn TableImpl::setColumn(Controler* plugin, tColId colId , tColType type, const StringRef& name) {
		ObjLocker lock(this, lockWrite);
		K_ASSERT(_dt.getRowCount() == 0); // nie mo¿emy u¿yæ getRowCount bo jest w nim assertLoaded!
		oColumn col = _dt.getColumn(colId);
		if (col->isUndefined() == false && col->hasFlag(cflagIsLoaded) == false) {
			IMessage(&sIMessage_plugOut(plugin->ID(), stringf("Kolumna %d w %s jest ju¿ ZAJÊTA!" , colId , this->getTableName().c_str()).c_str(), sIMessage_plugOut::erYes));
			//CtrlEx->PlugOut(plugin->getPluginId() ,  , 0);
			return oColumn();
		}
		return _dt.setColumn(colId, type, name);
	}

	oRow TableImpl::addRow(tRowId rowId) {
		oRow row;
		{
			ObjLocker lock(this, lockWrite);
			this->assertLoaded();
			row = _dt.addRow(rowId).get();
		}
		this->broadcastRowEvent(IM::rowAdded, row->getId());
		return row;
	}

	bool TableImpl::removeRow(tRowId rowId) {
		{
			ObjLocker lock(this, lockDefault);
			this->assertLoaded();
		}
		this->broadcastRowEvent(IM::rowRemoving, rowId);
		bool r;
		{
			Stamina::ObjLocker lock(this, lockDefault);
			this->assertLoaded(); // na wszelki :]
			r = _dt.deleteRow(rowId);
		}
		this->broadcastRowEvent(IM::rowRemoved, rowId);
		return r;
	}

	void TableImpl::reset() {
		this->broadcastEvent(IM::resetting);
		{
			Stamina::ObjLocker lock(this, lockDefault);
			this->_loaded = false;
			this->_columnsSet = false;
			this->_dt.clearRows();
			this->_dt.clearColumns();
		}
	}

	void TableImpl::resetData() {
		this->broadcastEvent(IM::resetting);
		{
			Stamina::ObjLocker lock(this, lockDefault);
			this->_dt.clearRows();
		}
	}

	void TableImpl::unloadData() {
		if (this->getOpt(optAutoSave)) {
			this->save();
		}
		{
			Stamina::ObjLocker lock(this, lockDefault);
			this->_loaded = false;
			this->_dt.clearRows();
		}
	}

	enResult TableImpl::load(bool force, const StringRef& filepath) {
		/*TODO: sprawdzanie poprawnosci hasel*/
		if (!force && this->isLoaded()) return errAlreadyLoaded;
		enResult result;
		{
			Stamina::ObjLocker lock(this, lockDefault);
			String file = (filepath.empty() == false) ? filepath : this->getFilepath();
			file = Stamina::expandEnvironmentStrings(file.c_str());
			result = errFileNotFound;
			if (!file.empty()) {
				FileBin fb;
				fb.warningDialogs = false;
				fb.assign(_dt);

				mainLogger->log(Stamina::logFunc, "Tables", "load", "%s", file.c_str());

				if (getOpt(optMakeBackups)) {
					fb.makeBackups = true;
				}

				if (this->getOpt(optDiscardLoadedColumns) == false && this->_columnsSet == false) {
					result = fb.loadAll(file);
				} else {
					result = fb.load(file);
				}
			}
			this->_columnsSet = true;
			this->_loaded = true;
		}
		if (result == success) 
			this->broadcastEvent(IM::afterLoad);
		return result;
	}


	enResult TableImpl::save(bool force, const StringRef& filepath) {
		if ((!force && _dt.isChanged() == false)) return errNotChanged;
		if (_columnsSet == false || (getOpt(optAutoLoad) && !isLoaded())) return errNotLoaded;
		_lateSaveTimer.reset();
		this->broadcastEvent(IM::beforeSave);
		enResult result;
		{
			Stamina::ObjLocker lock(this, lockDefault);
			String file = (filepath.empty() == false) ? filepath : this->getFilepath();
			file = Stamina::expandEnvironmentStrings(file.c_str());
			if (file.empty()) { 
				IMDEBUG(DBG_ERROR, "iTable::save() - no path to file!");
				return errFileNotFound;
			}
			FileBin fb(_dt);
			fb.warningDialogs = false;

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

			mainLogger->log(Stamina::logFunc, "Tables", "save", "%s", file.c_str());

			result = fb.save(file, op);
		}
		return result;
	}

	void TableImpl::lateSave(bool enabled) {
		Stamina::ObjLocker lock(this, lockDefault);
		if (enabled == false)
			_lateSaveTimer.reset();
		else
			Stamina::threadInvoke(mainThread, boost::bind(TableImpl::queryLateSave, this), false);

		IMDEBUG(DBG_FUNC, "iTable::lateSave(%d)", enabled);
	}

	void TableImpl::queryLateSave() {
		Stamina::ObjLocker lock(this, lockDefault);
		IMDEBUG(DBG_FUNC, "iTable::lateSave() queried");
		_lateSaveTimer.reset(Stamina::timerTmplCreate(boost::bind(TableImpl::onLateSaveTimer, this)));
		_lateSaveTimer->start(lateSaveDelay);
	}

	void TableImpl::onLateSaveTimer() {
		IMDEBUG(DBG_FUNC, "iTable::lateSave() invoked");
		this->save();
	}

	bool TableImpl::setOpt(enTableOptions option , bool enabled) {
		Stamina::ObjLocker lock(this, lockDefault);
		if (getOpt(option) == enabled) return enabled;
		if (enabled) {

			this->_opt = (enTableOptions)(this->_opt | option);
		} else {
			this->_opt = (enTableOptions)(this->_opt & (~option));
		}
		if (option & optGlobalData) {
			this->setDirectory();
		}

		if (option & optUseCurrentPassword) {
			this->_dt.setPasswordDigest(passwordDigest);
		}

		if (option & optSilent) {
			this->_dt.setInterface(enabled ? ifaceSilent.get() : iface.get());
		}

		return !enabled;
	}

	void TableImpl::setDirectory(const StringRef& path) {
		Stamina::ObjLocker lock(this, lockDefault);
		if (path.empty() == false) {
			this->_directory = unifyPath( path, false );
		} else {
			if (this->getOpt(optGlobalData)) {
				this->_directory = "%KonnektData%\\settings";
			} else {
				this->_directory = "%KonnektProfile%";
			}
		}
		this->_directory = expandEnvironmentStrings(this->_directory.a_str(), MAX_PATH);
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

	void TableImpl::destroy() {
		if (this->getOpt(optAutoSave)) {
			this->save();
		}
		__super::destroy();
	}

	bool TableImpl::unregisterTable() {
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

		// do listy znanych hase³...
		addPasswordDigest(digest);
	}

	void TableList::addPasswordDigest(const Stamina::MD5Digest& digest) {
		iface->addDigest(digest);
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
			t->setOpt(optBroadcastEvents, false);
			t->setOpt(optAutoLoad, true);
			t->setOpt(optGlobalData, true);
			t->setOpt(optDiscardLoadedColumns, true);
			t->setOpt(optUseCurrentPassword, true);
			t->setOpt(optUsePassword, false);
			testResult("optionSet", true, t->getOpt(optGlobalData));
			testResult("optionUnSet", 0, (int)(t->getOpt(optUsePassword)));
			testResult("optionComplex", 0, (int)(t->getOpt(optUseCurrentPassword)));
			t->setFilename("test_table.dtb");
			IMDEBUG(DBG_TEST, "filepath = %s \\ %s", t->getDirectory().c_str(), t->getFilename().c_str());
			IMDEBUG(DBG_TEST_TITLE, "setColumn");
			testResult("byId", 1, t->setColumn(1, ctypeInt, "col1")->setInt(rowDefault, 1));
			testResult("byName", colNotFound, t->setColumn(colByName, ctypeString, "col2")->setString(rowDefault, "TEST"), true);
			tColId col = t->getColumn("col2")->getId();
			testResult("getColId", colNotFound, col, true);
			testResult("getStr (autoLoad)", "BlaBlaBla", t->getString(0, "col2"));
			if (t->getRowCount()==0) {
				testResult("addRow", 0, t->addRow(), true);
			}
			testResult("setStr", true, t->setString(0, "col2", "Testunek"));
			testResult("save", true, t->save());
			IMDEBUG(DBG_TEST_TITLE, "unload");
			t->unloadData();
			testResult("getStr (autoLoad)", "Testunek", t->getString(0, "col2"));
			IMDEBUG(DBG_TEST_TITLE, "loadAll");
			t->setOpt(optDiscardLoadedColumns, false);
			t->reset();
			t->setColumn(2, ctypeInt)->setInt(rowDefault, 0);
			t->load();
			testResult("colCount", 3, t->getColCount());
			testResult("colId", "col2", t->getColumnByPos(2)->getName());
			IMDEBUG(DBG_TEST_TITLE, "release");
			t.set(0);
			IMDEBUG(DBG_TEST_TITLE, "set from id");
			t = oTable(ttId);
			testResult(true, (bool)t);
			testResult("getStr", "Testunek", t->getString(0, "col2"));

			// find...
			tRowId r1 = t->addRow();
			tRowId r2 = t->addRow();
			tRowId r3 = t->addRow();

			t->setInt(r1, "col1", 100);
			t->setInt(r2, "col1", 200);
			t->setInt(r3, "col1", 300);
			t->setString(r1, "col2", "a");
			t->setString(r2, "col2", "b");
			t->setString(r3, "col2", "c");

			IMDEBUG(DBG_TEST_TITLE, "find eqStr 'b'");
			testResult(r2, t->findRow(0, Find::EqStr(t->getColumn("col2"), "b")));
			IMDEBUG(DBG_TEST_TITLE, "find neqStr 'b' start r2");
			testResult(r3, t->findRow(r2, Find(Find::neq, t->getColumn("col2"), new Value_string("b"))));

			IMDEBUG(DBG_TEST_TITLE, "find neqStr 'a' AND col1 >= 100");
			testResult(r2, t->findRow(0, Find(Find::neq, t->getColumn("col2"), new Value_string("a")), Find(Find::gteq, t->getColumn("col1"), new Value_int(100))));

			IMDEBUG(DBG_TEST_TITLE, "find eqStr 'a' AND col1 > 100");
			testResult(rowNotFound, t->findRow(0, Find::EqStr(t->getColumn("col2"), "a"), Find(Find::gt, t->getColumn("col1"), new Value_int(100))));

			t->removeRow(r1);
			t->removeRow(r2);
			t->removeRow(r3);

			t.set(0);



			{
				Tables::oTable t2 = tables.registerTable(Ctrl->getPlugin(), tableNotFound, optDefaultSet);
				testResult("unregistered", true, (bool)t2);
				t2->setOpt( optBroadcastEvents, false );
				t2->setOpt( optAutoSave, true );
				t2->setOpt( optGlobalData, true );
				t2->setFilename("test_table.dtb");
				testResult("load", true, t2->load());
				testResult("setStr", true, t2->setString(0, "col2", "BlaBlaBla"));
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