#include "stdafx.h"

#include <Stamina\UI\Image.h>

#include "plugins.h"
#include "imessage.h"
#include "resources.h"
#include "plugins_ctrl.h"
#include "tables.h"
#include "tools.h"
#include "profiles.h"
#include "threads.h"
#include "argv.h"
#include "klogger.h"

using namespace Stamina;

namespace Konnekt {

	Plugins plugins;

	tTableId tablePlugins;

	VersionControl apiVersions;

	void initializePluginsTable() {
		using namespace Tables;
		oTable plg = registerTable(Ctrl, "Plugins", optPrivate | optAutoLoad | optAutoSave | /*optAutoUnload |*/ optDiscardLoadedColumns | optMakeBackups | optUseCurrentPassword);
		plg->setFilename("plg.dtb");
		plg->setDirectory();

		tablePlugins = plg->getTableId();

		plg->setColumn(PLG::file, ctypeString | cflagXor);
		plg->setColumn(PLG::md5, ctypeString | cflagXor);
		plg->setColumn(PLG::sig, ctypeString | cflagXor);
		plg->setColumn(PLG::load, ctypeInt);
		plg->setColumn(PLG::isNew, ctypeInt | cflagDontSave)->setInt(rowDefault, -1);
	}


	// --------------------------------------------------------------------

	Plugin::Plugin(tPluginId pluginId) {
		_imessageProc = 0;
		_imessageObject = 0;
		_module = 0;
		_running = false;
		_id = pluginId;
		_priority = priorityNone;
		_ctrl = 0;
		_owner = 0;
		_logger = new KLogger(*this, logAll | DBG_SPECIAL);
	}


	Plugin::~Plugin() {
		if (_ctrl) {
			delete _ctrl;
		}
		if (_module && !isVirtual()) {
			FreeLibrary(_module);
		}
	}

	int Plugin::getPluginIndex() {
		return plugins.getIndex(this->_id);
	}


	void Plugin::initVirtual(Plugin& owner, void* imessageObject, void* imessageProc) {
		_owner = &owner;
		_imessageObject = imessageObject;
		_imessageProc = imessageProc;
		_file = _owner->_file;
		_module = _owner->_module;
		_version = _owner->_version;
		this->initData();
	}

	void Plugin::initClassic(const StringRef& file, void* imessageProc) {
		_owner = 0;
		_imessageObject = 0;
	
		_file = file;
		FileVersion fv(file);
		this->_version = fv.getFileVersion();

		_module = LoadLibrary(file.a_str());

		if (!_module) {
			string err = "B³¹d podczas ³adowania [";
			int errNo = GetLastError();
			err += inttostr(errNo);
			err += "] ";
			err += getErrorMsg(errNo);
			//strcpy(TLS().buff , err.c_str());
			throw ExceptionString(err);
		}

		if (imessageProc) {
			_imessageProc = imessageProc;
		} else {
			_imessageProc = (fIMessageProc)GetProcAddress(_module , "IMessageProc");
		}

		if (!_imessageProc) {
			_imessageProc=(fIMessageProc)GetProcAddress(_module , "_IMessageProc@4");
			if (!_imessageProc) {
				throw ExceptionString("Ten plik najprawdopodobniej nie jest wtyczk¹!");
			}
		}

		this->initData();

	}

	void Plugin::initData() {
		_originalObject = _imessageObject;
		_originalProc = _imessageProc;
		static int uniqueId = pluginsDynamicIdStart;
		if (this->_id == pluginNotFound) {
			this->_id = (tPluginId)uniqueId++;
		}
		this->_debugColor = Stamina::UI::getUniqueColor(plugins.count(), 4, 0xFF, true, true);
		this->_running = false;
		this->_priority = priorityNone;

		this->_type = (enIMessageType) this->IMessageDirect(IM_PLUG_TYPE, 0, 0);
		if (this->_version.empty()) {
			this->_version = Version( safeChar(this->IMessageDirect(IM_PLUG_VERSION, 0, 0)) );
		}
		this->_sig = safeChar(this->IMessageDirect(IM_PLUG_SIG, 0 ,0));
		this->_net = (tNet)this->IMessageDirect(IM_PLUG_NET, 0 ,0);
		this->_name = safeChar(this->IMessageDirect(IM_PLUG_NAME, 0 ,0));
		this->_netName = safeChar(this->IMessageDirect(IM_PLUG_NETNAME, 0 ,0));
		this->_netShortName = safeChar(this->IMessageDirect(IM_PLUG_NETSHORTNAME, 0 ,0));
		this->_uidName = safeChar(this->IMessageDirect(IM_PLUG_UIDNAME, 0 ,0));

		if (this->IMessageDirect(IM_PLUG_SDKVERSION, 0 ,0) < KONNEKT_SDK_V) { 
			throw ExceptionString("Wtyczka przygotowana zosta³a dla starszej wersji API. Poszukaj nowszej wersji!");
		}

		if ((this->getId() != pluginCore && this->getId() != pluginUI && _net == Net::none) || _net >= Net::last) throw ExceptionString("Z³a wartoœæ NET!");
		if (_sig == "" || _name == "") throw ExceptionString("Brakuje nazwy, lub wartoœci SIG!");

		if (RegEx::doMatch("/^[A-Z0-9_]{2,12}$/i", _sig.a_str()) == 0) throw ExceptionString("Wartoœæ SIG nie spe³nia wymagañ!");

		if (plugins.findSig(_sig) != 0) {
			throw ExceptionString("Wartoœæ SIG jest ju¿ zajêta!");
		}

	}


	Plugin* versionComparing;

	void __stdcall versionCompare(const Stamina::ModuleVersion& v) {

		Version local = apiVersions.getVersion(v.getCategory(), v.getName());

		if (local.empty()) return;

		versionComparing->getLogger()->log(logDebug, "apiVersion", 0, "%s (%s) %s [%d]"
			, v.getVersion().getString(4).c_str()
			, local.getString(4).c_str()
			, v.getName()
			, v.getCategory()
			);

		if (v.getVersion().major != local.major || v.getVersion().minor > local.minor) {

			throw ExceptionString( stringf("Niezgodnoœæ wersji komponentu \"%s\"! Lokalna: %s; Wtyczki: %s"
				, v.getName()
				, local.getString(4).c_str()
				, v.getVersion().getString(4).c_str()
				));

		}

	}

	void __stdcall versionRegister(const Stamina::ModuleVersion& v) {
		apiVersions.registerModule(v);
	}


	void Plugin::checkApiVersions(bool registerVersions) {
		typedef void (__stdcall *fComparer)(fApiVersionCompare cmp);

		fComparer comparer;

		comparer = (fComparer)GetProcAddress(_module , "KonnektApiVersions");
		if (!comparer) {
			comparer = (fComparer)GetProcAddress(_module , "_KonnektApiVersions@4");
		}
		
		if (comparer) {
			versionComparing = this;
			comparer(versionCompare);
		}

		if (registerVersions) {
			fComparer registrar;

			registrar = (fComparer)GetProcAddress(_module , "KonnektApiRegister");
			if (!registrar) {
				registrar = (fComparer)GetProcAddress(_module , "_KonnektApiRegister@4");
			}
			
			if (registrar) {
				registrar(versionRegister);
			}
		}


	}


	void Plugin::run() {
		if (this->getId() == pluginCore) {
			this->_ctrl = createCorePluginCtrl(*this);
			//Ctrl = this->_ctrl; // musimy to ustawiæ zanim u¿yjemy któregoœ IMessageDirect!
		} else if (this->getId() == pluginUI) {
			this->_ctrl = createCorePluginCtrl(*this);
		} else {
			this->_ctrl = createPluginCtrl(*this);
		}

		this->_running = true;
		if (this->IMessageDirect(IM_PLUG_INIT, (int)this->_ctrl, (int)this->_id) == 0) {
			throw ExceptionString("Inicjalizacja siê nie powiod³a!");
		}
	}

	void Plugin::deinitialize() {
		if (!isRunning()) return;
		Ctrl->IMessage(IM_PLUG_PLUGOUT, Net::broadcast, imtAll, this->getId());

		// wywalamy jego wirtualne i restujemy subclassowanie...
		for (Plugins::tList::iterator it = plugins.begin(); it != plugins.end(); ++it) {
			if ((*it)->getLastSubclasser().get() == this) {
				(*it)->resetSubclassing();
			}
			if ((*it)->getOwnerPlugin() == this) {
				plugins.plugOut(*(*it), false);
			}
		}

		this->resetSubclassing();

		bool nofree = this->IMessageDirect(IM_PLUG_DONTFREELIBRARY, 0 ,0);
		this->_running=false;

		this->IMessageDirect(IM_PLUG_DEINIT, 0 ,0);
		if (nofree) {
			_module = 0;
		}
	}




	bool Plugin::plugOut(Controler* sender, const StringRef& reason, bool quiet, enPlugOutUnload unload) {
		Stamina::mainLogger->log(Stamina::logWarn, "Plugin", "plugOut", "unload=%d reason=\"%s\"", unload, reason.a_str());

		if (mainThread.isCurrent() == false) {
			threadInvoke(mainThread, boost::bind(&Plugin::plugOut, this, sender, reason, quiet, unload));
			return true;
		}

		Tables::oTableImpl plg(tablePlugins);
		tRowId pl = plg->findRow(0, DT::Find::EqStr(plg->getColumn(PLG::file), this->getDllFile())).getId();
		if (pl == DT::rowNotFound) return false; // dziwna sprawa :)
		if (unload & iPlugin::poUnloadOnNextStart) {
			plg->setInt(pl , PLG::load , -1);
			plg->save();
		}

		String namePlug = this->getName();
		String nameSender = sender->getPlugin()->getName();
		
		bool result = true;

		if (unload & iPlugin::poUnloadNow) {
			if (this->canPlugOut()) {
				plugins.plugOut(*this, true);
			} else {
				result = false;
			}
		}

		if (!quiet) {
			CStdString msg = loadString(IDS_INF_PLUGOUT);
			msg.Format(msg, nameSender, namePlug, reason, "");
			MessageBox(NULL, msg, loadString(IDS_APPNAME).c_str(), MB_ICONWARNING|MB_TASKMODAL|MB_OK);
		}

		
		return result;
	}

	bool Plugin::subclassIMessageProc(Controler* sender, void*& proc, void*& object) {
		if (!sender) return false;

		void * tempProc = proc;
		void * tempObject = object;

		proc = _imessageProc;
		object = _imessageObject;

		if (tempProc) {
			_imessageProc = tempProc;
			_imessageObject = tempObject;
		}
		return true;
	}


	void Plugin::madeError(const StringRef& msg , unsigned int severity) {
		String info = String(L"Wtyczka \"") + this->_name + L"\" zrobi³a bubu:\r\n" + msg + L"\r\n\r\nPrzejmujemy siê tym?";
		if (MessageBoxW(0 , info.w_str() , L"Konnekt" , MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_YESNO) == IDYES)
		{ 
			Tables::oTableImpl plg(tablePlugins);
			tRowId pl = plg->findRow(0, DT::Find::EqStr(plg->getColumn(PLG::file), this->_file)).getId();
			if (pl == DT::rowNotFound) return;
			plg->setInt(pl , PLG::load , -1);
			plg->save();
			if (!Konnekt::running) {
				deinitialize();
			} else {
				deinitialize();
				restart();
			}
		}
	}


	// --------------------------------------------------------------------

	int Plugins::getIndex(tPluginId id) {
		if (id < pluginsMaxCount) {
			if ((unsigned)id >= _list.size()) {
				return pluginNotFound;
			}
			return id;
		}
		for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
			if ((*it)->getId() == id) return it - _list.begin();
		}
		return pluginNotFound;
	}

	tPluginId Plugins::getId(int id) {
		if (id > pluginsMaxCount) {
			return (tPluginId)id;
		}
		if (id > 0 && id < _list.size()) {
			return _list[id]->getId();
		} else {
			return pluginNotFound;
		}
	}


	int Plugins::findPlugin(tNet net , enIMessageType type , unsigned int start) {
		for (tList::iterator it = _list.begin() + start; it != _list.end(); ++it) {
			if ((type == -1 || ((*it)->_type & type)==type) && (net == Net::broadcast || net == Net::first || (*it)->_net == net)) {
				return it - _list.begin();
			}
		}
		return pluginNotFound;
	}

	Plugin* Plugins::findName(const StringRef& name) {
		for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
			if ((*it)->getName().equal(name, true)) {
				return &*(*it);
			}
		}
		return 0;
	}


	Plugin* Plugins::findSig(const StringRef& sig) {
		for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
			if ((*it)->getSig().equal(sig, true)) {
				return &*(*it);
			}
		}
		return 0;
	}



	String Plugins::getName(tPluginId id) {
		if (id == pluginNotFound) return "Unknown";
		int pos = this->getIndex(id);
		if (pos >= 0) {
			return _list[pos]->getName();
		} else {
			return stringf("%#.4x",id);
		}
	}

	String Plugins::getSig(tPluginId id) {
		if (id == pluginNotFound) return "Unknown";
		int pos = this->getIndex(id);
		if (pos >= 0) {
			return _list[pos]->getSig();
		} else {
			return stringf("%#.4x",id);
		}
	}


	void Plugins::plugInClassic(const StringRef& filename, void* imessageProc, tPluginId pluginId) {
		ptrPlugin plugin (new Plugin(pluginId));
		try {
			plugin->initClassic(filename, imessageProc);
			plugin->checkApiVersions(true);
			plugin->run();
			_list.push_back(plugin);
		} catch (ExceptionString& e) {
			mainLogger->log(logError, "Plugins", "plugInClassic", "\"%s\" failed: \"%s\"", filename.a_str(), e.getReason().a_str());
			throw e;
		}
	}

	void Plugins::plugInVirtual(Plugin& owner, void* object, void* proc, tPluginId pluginId) {
		ptrPlugin plugin (new Plugin(pluginId));
		try {
			plugin->initVirtual(owner, object, proc);
			plugin->run();
			_list.push_back(plugin);
		} catch (ExceptionString& e) {
			mainLogger->log(logError, "Plugins", "plugInVirtual", "\"%s\" failed: \"%s\"", owner.getName().a_str(), e.getReason().a_str());
			throw e;
		}
	}

	bool Plugins::plugOut(Plugin& plugin, bool removeFromList) {
		plugin.deinitialize();
		if (removeFromList) {
			bool fPlug = false;

			for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
				if (it->get() == &plugin) {
					fPlug = true;
					break;
				}
			}
			plugins.cleanup(); // wywalamy to co ew. zosta³o wypiête w deinitialize
			/*
			for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
				if (it->get() == p) {
					_list.erase(it);
					return true;
				}
			}
			*/
			if (fPlug) return true;
			bool pluginOnList = false;
			S_ASSERT(pluginOnList == true);
			return false;
		}
		return true;
	}

	void Plugins::cleanup() {
		tList::iterator it = _list.begin();
		while (it != _list.end()) {
			if ((*it)->isRunning() == false) {
				it = _list.erase(it);
			} else {
				++it;
			}
		}
	}

	void Plugins::sortPlugins(void) {
		// Na poczatku wszystkie wtyczki maja 0 dziêki czemu bêdziemy je
		// trzymaæ na samym koñcu listy przez ca³y czas...
		tList::iterator it = _list.begin();
		tList newList;
		for (it = _list.begin(); it != _list.end(); ++it) {
			ptrPlugin current = *it;
			Plugin* plugin = &*current;
			if (plugin->getId() == pluginCore || plugin->getId() == pluginUI) {
				plugin->_priority = priorityCore;
			} else {
				// sprawdzamy priorytety
				plugin->_priority = (enPluginPriority)plugin->IMessageDirect(IM_PLUG_PRIORITY, 0 ,0);
				if (!plugin->_priority || plugin->_priority > priorityHighest) {
					plugin->_priority = priorityStandard;
				}
			}

			// szukamy teraz, gdzie go wrzucic... na pewno gdzies wczesniej
			tList::iterator ins = newList.begin();
			while (ins != newList.end() && (*ins)->getPriority() >= plugin->_priority) {
				ins++;
			}

			newList.insert(ins , current);
		}
		_list = newList;
	}

	void Plugins::cleanUp() {
		_list.clear();
	}

	// --------------------------------------------------------------------


	void Plugins::checkVersions(void) {

		using namespace Tables;
		oTable state = registerTable(Ctrl, "PluginState", optPrivate |  optDiscardLoadedColumns);
		state->setFilename("plg_state.dtb");
		state->setDirectory();

		state->setColumn("sig", ctypeString);
		state->setColumn("file", ctypeString);
		state->setColumn("version", ctypeInt);
		state->setColumn("last", ctypeInt64);
		state->load();

		bool useOldMethod = (state->getRowCount() == 0);

		string ver;
		if (useOldMethod) {
			ver = Tables::cfg->getString(0 , CFG_VERSIONS);
			if (ver.empty()) {
				useOldMethod = false;
			} else {
				unsigned int prevVer = findVersion(ver , "CORE" , versionNumber);
				if (versionNumber > prevVer) updateCore(prevVer);
			}
		}

		for (int i = 0; i < plugins.count(); i++) {
			if (plugins[i].isVirtual()) continue;

			oRow row = state->findRow(0, DT::Find::EqStr( state->getColumn("sig"), plugins[i].getSig() ));

			if (row.isValid() == false) {
				row = state->addRow();
				state->setString(row, "sig", plugins[i].getSig());
			}

			int prevVer;

			if (useOldMethod) {
				prevVer = findVersion(ver, plugins[i].getDllFile().toLower() , plugins[i].getVersion().getInt());
			} else {
				prevVer = state->getInt(row, "version");
			}

			state->setInt(row, "version", plugins[i].getVersion().getInt());
			state->setString(row, "file", plugins[i].getDllFile().toLower());
			state->setInt64(row, "last", Stamina::Time64(true));

			if (plugins[i].getVersion().getInt() > prevVer) {
				plugins[i].IMessageDirect(IM_PLUG_UPDATE , prevVer, 0);
			}
		}
		Tables::cfg->setString(0 , CFG_VERSIONS , "");
		Tables::cfg->save();

		state->save();
		state->unregisterTable();
	}



	struct FailedPlugin {
		FailedPlugin(StringRef cause, StringRef filename) {
			this->cause = cause;
			this->filename = filename;
			this->plugin = 0;
		}
		FailedPlugin(StringRef cause, Plugin* plugin) {
			this->cause = cause;
			this->filename = plugin->getDllFile();
			this->plugin = plugin;
		}
		String cause;
		String filename;
		Plugin* plugin;
	};


	void Plugins::setPlugins(bool noDlg , bool startUp) {
		string plugDir = loadString(IDS_PLUGINDIR);
		int i = 0;

		FindFile ff(plugDir + "*.dll");
		ff.setFileOnly();
        
		bool newOne = false;
		Tables::oTableImpl plg(tablePlugins);
		while (ff.find())
		{
			if (!RegEx::doMatch("/^[A-Za-z].*\\.dll$/", ff.found().getFileName())) continue;
			if (_stricmp("ui.dll" , ff.found().getFileName().c_str())) {
				tRowId p = plg->findRow(0, DT::Find::EqStr(plg->getColumn(PLG::file), plugDir + ff.found().getFileName())).getId();
					if (p == DT::rowNotFound) {
						newOne = true;
						p = plg->addRow();
						plg->setInt(p , PLG::isNew , 1);
					} else plg->setInt(p , PLG::isNew , 0);
					plg->setString(p , PLG::file , plugDir + ff.found().getFileName());
					i++;
				}
		}

		if ((startUp && argVExists(ARGV_PLUGINS)) || (newOne && !noDlg))
			PlugsDialog(Konnekt::newProfile);
		if (!startUp) return;
		newOne = false;
		// Wlaczanie wtyczek:

		int apiVersionsCount = apiVersions.size();


		typedef std::list<FailedPlugin> tFailedPlugins;
		tFailedPlugins failedPlugins;

		for (unsigned int i = 0 ; i < plg->getRowCount(); i++) {
			if (plg->getInt(i , PLG::isNew)==-1) {
				plg->removeRow(i);
				i--;
				continue;
			} // usuwa nie istniejaca wtyczke
			if (plg->getInt(i , PLG::load) !=1 ) 
				continue;
			String file = plg->getString(i , PLG::file);
			try {
				plugins.plugInClassic(file);
			} catch (Exception& e) {

				mainLogger->log(logError, "setPlugins", "plugInClassic", "Wyst¹pi³ b³¹d podczas ³adowania %s - \"%s\"", file.c_str(), e.getReason().c_str());
				failedPlugins.push_back( FailedPlugin(e.getReason(), file) );

			}
		}

		if (apiVersionsCount != apiVersions.size()) {
			mainLogger->log(logLog, "Plugins", "setPlugins", "New API classes detected! rechecking versions...");
			for (Plugins::tList::iterator it =  plugins.begin(); it != plugins.end(); ++it) {
				try {
					if ((*it)->isVirtual()) continue;
					(*it)->checkApiVersions(false);

				} catch (Exception& e) {

					mainLogger->log(logError, "setPlugins", "plugInClassic", "Wyst¹pi³ b³¹d podczas sprawdzania wersji %s - \"%s\"", (*it)->getName().c_str(), e.getReason().c_str());
					failedPlugins.push_back( FailedPlugin(e.getReason(), it->get()) );

				}

			}
		}

		if (failedPlugins.size()) {

			String msg = "Wyst¹pi³y b³êdy podczas ³adowania wtyczek.\r\nJe¿eli naciœniesz [TAK] pluginy te nie bêd¹ ³adowane ponownie!\r\n\r\n";

			for (tFailedPlugins::iterator it = failedPlugins.begin(); it != failedPlugins.end(); ++it) {
				;
				msg += getFileName(it->filename);
				msg += " : \"";
				msg += it->cause;
				msg += "\"\r\n";

				if (it->plugin) { // wy³adowywujemy...
					plugins.plugOut(*it->plugin, true);
				}
			}

			int ret = MessageBox(NULL , msg.c_str() , loadString(IDS_APPNAME).c_str() , MB_ICONERROR|MB_YESNOCANCEL|MB_TASKMODAL);
			if (ret == IDCANCEL) abort();
			if (ret == IDYES) { // ¿eby siê wiêcej nie ³adowa³y...
				for (tFailedPlugins::iterator it = failedPlugins.begin(); it != failedPlugins.end(); ++it) {
					tRowId id = plg->findRow(0, DT::Find::EqStr(plg->getColumn(PLG::file), it->filename)).getId();
					plg->setInt(id , PLG::load , -1);
					newOne = 1;
				}
			}
		}

        plg->save();
		IMLOG("- Plug.sort");
		plugins.sortPlugins();
		//  if (newOne)
		//    IMessageDirect(Plug[0] , IMI_DLGPLUGS , 2);


	}


	// ---------------------------------------------





};