#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include <Stamina\ObjectImpl.h>
// Info o wtyczce


namespace Konnekt {

	namespace PLG {
		const tColId file = 0;
		const tColId md5 = 1;
		const tColId sig = 2;
		const tColId load = 3;
		const tColId isNew = 4;
	};

	extern tTableId tablePlugins;

	void pluginsInit();





	class Plugin: public LockableObject<iPlugin> {
	public:

		friend class Plugins;

		STAMINA_OBJECT_CLASS(Konnekt::Plugin, Stamina::iPlugin);

		Plugin(tPluginId pluginId = pluginNotFound) {
			_imessageProc = 0;
			_imessageObject = 0;
			_module = 0;
			_running = false;
			_id = pluginId;
			_priority = priorityNone;
			_ctrl = 0;
			_owner = 0;
		}
		~Plugin();

		virtual tPluginId __stdcall getPluginId() {
			return this->_id;
		}

		virtual int __stdcall getPluginIndex() {
			return plugins.getIndex(this->_id);
		}

		virtual HMODULE __stdcall getDllInstance() {
			return _module;
		}

		virtual const String& __stdcall getDllFile() {
			return _file;
		}

		virtual iPlugin* __stdcall getOwnerPlugin() {
			return _owner;
		}

		virtual tNet __stdcall getNet() {
			return this->_net;
		}

		virtual enIMessageType __stdcall getType() {
			return this->_type;
		}

		virtual Stamina::Version __stdcall getVersion() {
			return this->_version;
		}

		virtual const String& __stdcall getSig() {
			return this->_sig;
		}

		virtual const String& __stdcall getName() {
			return this->_name;
		}

		virtual const String& __stdcall getNetName() {
			return this->_netName;
		}

		virtual enPlugPriority __stdcall getPriority() {
			return this->_priority;
		}

		virtual bool __stdcall canHotPlug() {
			return false;
		}

		virtual bool __stdcall canPlugOut() {
			return this->canHotPlug() || Konnekt::running == false;
		}

		int sendIMessage(cCtrl* sender, sIMessage_base* im) {
			im->sender = sender->ID();
			this->sendIMessage(im);
		}

		int sendIMessage(sIMessage_base* im);

		virtual bool __stdcall plugOut(const cCtrl* sender, const StringRef& reason, bool quiet, enPlugOutUnload unload);

		COLORREF getDebugColor() {
			return _debugColor;
		}

		cCtrl_* getCtrl() {
			return _ctrl;
		}

		virtual bool __stdcall isRunning() {
			return _running;
		}

		void madeError(const StringRef& msg , unsigned int severity);

		int IMessage(tIMid id, int p1 = 0, int p2 = 0) {
			return this->sendIMessage(Ctrl, &sIMessage_2params(id, p1, p2));
		}


	private:

		int callIMessageProc(sIMessage_base*im);


		void initClassic(StringRef& file, void* imessageProc = 0);
		void initVirtual(Plugin& owner, void* imessageObject, void* imessageProc);
		void initData();
		void run();

		void deinitialize();

	private:

		void* _imessageProc;
		void* _imessageObject;
		String _file;
		HMODULE _module;
		tNet _net;
		enIMessageType _type;
		Stamina::Version _version;
		String _sig;
		String _name;
		String _netName;
		COLORREF _debugColor;
		bool _running;
		tPluginId _id;
		enPluginPriority _priority;

		class cCtrl_ * _ctrl;
		Plugin* _owner;

	};



	class Plugins {
	public:

		typedef boost::shared_ptr<Plugin> ptrPlugin;
		typedef deque < ptrPlugin > tList;

		int getIndex(tPluginId id);
		tPluginId getId(int id);

		Plugin* get(tPluginId id) {
			int pos = this->getIndex(id);
			if (pos == pluginNotFound) return 0;
			return &(*_list[pos]);
		}

		Plugin& operator [] (unsigned int i) {
			i = this->getId(i);
			if (i == pluginNotFound) {
				throw Stamina::ExceptionString("Plugin out of range");
			}
			return _list[i];
		}

		bool exists(unsigned int i) {
			i = this->getIndex(i);
			return (i != pluginNotFound && i >= 0 && i < _list.size());
		}

		int count() {
			return _list.size();
		}

		enum enPlugInResult {
			resultPlugged = 2,
			resultFailedRetry = 1,
			resultFailedNoRetry = 0,
		};

		void plugInClassic(const StringRef& filename, void* imessageProc = 0, tPluginId pluginId = pluginNotFound) throw(...);

		void plugInVirtual(Plugin& owner, void* object, void* proc, tPluginId pluginId = pluginNotFound) throw(...);

		bool plugOut(Plugin& plugin, bool removeFromList);

		int findPlugin(tNet net , enIMessageType type , unsigned int start=0);

		Plugin* findSig(const StringRef& sig);

		String getName(tPluginId id);

		void sortPlugins();

		tPluginId getId() {
			return _id;
		}

	private:

		tList _list;

	};

	extern Plugins plugins;


	void setPlugins(bool noDlg = false , bool startUp = true);
	void checkVersions(void);


	cCtrl_ * createPluginCtrl(Plugin& plugin);
	cCtrl_ * createCorePluginCtrl(Plugin& plugin);

	void PlugsDialog(bool firstTimer = false);

};