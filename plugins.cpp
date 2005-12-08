#include "stdafx.h"

#include <Stamina\Image.h>

#include "plugins.h"
#include "imessage.h"
#include "resources.h"
#include "plugins_ctrl.h"
#include "tables.h"
#include "tools.h"
#include "profiles.h"
#include "argv.h"

using namespace Stamina;

namespace Konnekt {

	Plugins plugins;

	tTableId tablePlugins;

	void pluginsInit() {
		using namespace Tables;
		oTable plg = registerTable(Ctrl, "Plugins", optPrivate | optAutoLoad | optAutoSave | optAutoUnload | optDiscardLoadedColumns | optMakeBackups | optUseCurrentPassword);
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

	Plugin::~Plugin() {
		if (_ctrl) {
			delete _ctrl;
		}
		if (_module && !isVirtual()) {
			FreeLibrary(_module);
		}
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

	void Plugin::initClassic(StringRef& file, void* imessageProc) {
		_owner = 0;
		_imessageObject = 0;
	
		_file = file;
		FileVersion fv(file);
		this->_version = fv.getFileVersion();

		_module = LoadLibrary(file);

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
		static uniqueId = pluginsDynamicIdStart;
		uniqueId++;
		if (this->_id == pluginNotFound) {
			this->_id = uniqueId;
		}
		this->_debugColor = getUniqueColor(plugins.count(), 4, 0xFF, true, true);
		this->_running = false;
		this->_priority = priorityNone;


		this->_type = (enIMessageType) this->IMessage(IM_PLUG_TYPE);
		if (this->_version.empty()) {
			this->_version = Version( safeChar(this->IMessage(IM_PLUG_VERSION)) );
		}
		this->_sig = safeChar(this->IMessage(IM_PLUG_SIG));
		this->_net = this->IMessage(IM_PLUG_NET);
		this->_name = safeChar(this->IMessage(IM_PLUG_NAME));
		this->_netName = safeChar(this->IMessage(IM_PLUG_NETNAME));

		if (this->IMessage(IM_PLUG_SDKVERSION) < KONNEKT_SDK_V) { 
			throw ExceptionString("Wtyczka przygotowana zosta³a dla starszej wersji API. Poszukaj nowszej wersji!");
		}

		if (this->getId() != pluginCore && this->getId() != pluginUI && dll.net==0) throw ExceptionString("Z³a wartoœæ NET!");
		if (_sig == "" || _name == "") throw ExceptionString("Brakuje nazwy, lub wartoœci SIG!");

		if (RegEx::doMatch("/^[A-Z0-9_]{3,12}$/i", _sig) == 0) throw ExceptionString("Wartoœæ SIG nie spe³nia wymagañ!");

		if (plugins.findSig(_sig) != 0) {
			throw ExceptionString("Wartoœæ SIG jest ju¿ zajêta!");
		}

	}

	void Plugin::run() {
		if (this->getId() == pluginCore || this->getId() == pluginUI) {
			this->_ctrl = createCorePluginCtrl(*this);
		} else {
			this->_ctrl = createPluginCtrl(*this);
		}
		this->_running = true;
		if (this->IMessage(IM_PLUG_INIT, (tIMP)this->_ctrl, (tIMP)this->_id, 0) == 0) {
			throw ExceptionString("Inicjalizacja siê nie powiod³a!");
		}
	}

	void Plugin::deinitialize() {
		IMessage(IM_PLUG_PLUGOUT , NET_BROADCAST , IMT_ALL , this->getId() , 0 );
		bool nofree = this->IMessage(IM_PLUG_DONTFREELIBRARY);
		this->_running=false;
		this->IMessage(IM_PLUG_DEINIT);
		if (nofree) {
			_module = 0;
		}
	}



	inline int Plugin::callIMessageProc(sIMessage_base*im) {
		if (_imessageObject == 0) {
			return ((fIMessageProc)_imessageProc)(im);
		} else {
			typedef int (__stdcall*fIMessageProcObject)(void*, sIMessage_base * msg);
			return ((fIMessageProcObject)_imessageProc)(_imessageObject, im);
		}
	}

	bool __stdcall Plugin::plugOut(const cCtrl* sender, const StringRef& reason, bool quiet, enPlugOutUnload unload) {
		Stamina::mainLogger->log(Stamina::logWarn, "Plugin", "plugOut", "unload=%d reason=\"%s\"", unload, reason.a_str());

		if (mainThread.isCurrent() == false) {
			threadInvoke(mainThread, boost::bind(Plugin::plugOut, this, sender, reason, quiet, unload));
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
				plugins.plugOut(*this);
			} else {
				result = false;
			}
		}

		if (!quiet) {
			CStdString msg = loadString(IDS_INF_PLUGOUT);
			msg.Format(msg, nameSender, namePlug, reason, "");
		}

		MessageBox(NULL, msg, loadString(IDS_APPNAME).c_str(), MB_ICONWARNING|MB_TASKMODAL|MB_OK);
		
		return result;
	}



	void Plugin::madeError(const StringRef& msg , unsigned int severity) {
		String info = L"Wtyczka \"" + this->_name + L"\" zrobi³a bubu:\r\n" + msg + L"\r\n\r\nPrzejmujemy siê tym?";
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
			(*_list[i])->getId();
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

	Plugin* Plugins::findSig(const StringRef& sig) {
		for (tList::iterator it = _list.begin() + start; it != _list.end(); ++it) {
			if ((*it)->getSig().equal(sig, true)) {
				return &(*it);
			}
		}
		return 0;
	}



	String Plugins::getName(tPluginId id) {
		if (h == 0) return "CORE";
		if (h < 3) return stringf("___BC%u", id - 1);
		int pos = this->getIndex(id);
		if (pos >= 0) {
			return _list[pos].getName();
		} else {
			return stringf("%#.4x",h);
		}
	}


	bool Plugins::plugInClassic(const StringRef& filename) {
	}

	bool Plugins::plugInVirtual(Plugin& owner, void* object, void* proc) {
	}

	bool Plugins::plugOut(Plugin& plugin) {
	}



	void Plugins::plugInClassic(const StringRef& filename, void* imessageProc, tPluginId pluginId) {
		ptrPlugin plugin (new Plugin(pluginId));
		try {
			plugin->initClassic(filename, imessageProc);
			plugin->run();
			_list.push_back(plugin);
		} catch (Exception& e) {
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
		} catch (Exception& e) {
			mainLogger->log(logError, "Plugins", "plugInVirtual", "\"%s\" failed: \"%s\"", owner.getName().a_str(), e.getReason().a_str());
			throw e;
		}
	}

	bool Plugins::plugOut(Plugin& plugin, bool removeFromList) {
		plugin.deinitialize();
		if (removeFromList) {
			for (tList::iterator it = _list.begin(); it != _list.end(); ++it) {
				if (it->get() == &plugin) {
					_list.erase(it);
					return true;
				}
			}
			bool pluginOnList = false;
			S_ASSERT(pluginOnList == true);
		}
	}


	void Plugins::sortPlugins(void) {
		// Na poczatku wszystkie wtyczki maja 0 dziêki czemu bêdziemy je
		// trzymaæ na samym koñcu listy przez ca³y czas...
		tList::iterator it = _list.begin();
		while (it != _list.end()) {
			ptrPlugin current = *it;
			Plugin* plugin = *current;
			if (plugin->getId() == pluginCore || plugin->getId() == pluginUI) {
				plugin->_priority = priorityCore;
				continue;
			}

			// sprawdzamy priorytety
			plugin->_priority = (enPluginPriority)plugin->IMessage(IM_PLUG_PRIORITY);
			if (!plugin->_priority || plugin->_priority > priorityHighest) {
				plugin->_priority = priorityStandard;
			}
			// usuwamy, ¿eby nie przeszkadza³...
			it = _list.erase(it);
			// szukamy teraz, gdzie go wrzucic... na pewno gdzies wczesniej
			tList::iterator ins = _list.begin();
			while (ins != _list.end() && (*ins)->getPriority() >= plugin->_priority) {
				ins++;
			}

			_list.insert(ins , current);
			if (it != _list.end()) {
				it ++;
			}
		}
	}


	// --------------------------------------------------------------------


	void checkVersions(void) {

		using namespace Tables;
		oTable state = registerTable(Ctrl, "PluginState", optPrivate |  optDiscardLoadedColumns | opt);
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
				state->setString("sig", plugins[i].getSig());
			}

			if (useOldMethod) {
				prevVer = findVersion(ver, plugins[i].getDllFile().toLower() , plugins[i].getVersion().getInt());
			} else {
				prevVer = row->getInt("version");
			}

			row->setInt("version", plugins[i].getVersion().getInt());
			row->setString("file", plugins[i].getDllFile().toLower());
			row->setInt64("last", Stamina::Time64(true));

			if (plugins[i].getVersion().getInt() > prevVer) {
				plugins[i].sendIMessage(Ctrl, &sIMessage_2params(IM_PLUG_UPDATE , prevVer));
			}
		}
		Tables::cfg->setString(0 , CFG_VERSIONS , "");
		Tables::cfg->save();

		state->save();
		state->unregisterTable();
	}



	void setPlugins(bool noDlg , bool startUp) {
		string plugDir = loadString(IDS_PLUGINDIR);
		int i = 0;

		FindFile ff(plugDir + "*.dll");
		ff.setFileOnly();
        
		bool newOne = false;
		Tables::oTableImpl plg(tablePlugins);
		while (ff.find())
		{
			if (stricmp("ui.dll" , ff.found().getFileName().c_str())) {
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

		if ((startUp && getArgV(ARGV_PLUGINS , false)) || (newOne && !noDlg))
			PlugsDialog(Konnekt::newProfile);
		if (!startUp) return;
		newOne = false;
		// Wlaczanie wtyczek:
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

				CStdString errMsg;
				errMsg.Format(loadString(IDS_ERR_DLL).c_str(), file.a_str() , e.getReason().a_str());
				int ret = MessageBox(NULL , errMsg , loadString(IDS_APPNAME).c_str() , MB_ICONWARNING|MB_YESNOCANCEL|MB_TASKMODAL);
				if (ret == IDCANCEL) abort();
				if (ret == IDYES) {
					plg->setInt(i , PLG::load , -1);
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