#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include <Stamina\ObjectImpl.h>
// Info o wtyczce

using namespace Stamina;

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

	class cCtrl_;

	class Plugin: public LockableObject<iPlugin> {
	public:

		friend class Plugins;

		STAMINA_OBJECT_CLASS(Konnekt::Plugin, Konnekt::iPlugin);

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

		virtual tPluginId getPluginId() {
			return this->_id;
		}

		virtual int getPluginIndex();

		virtual HMODULE getDllModule() {
			return _module;
		}

		virtual const String& getDllFile() {
			return _file;
		}

		virtual iPlugin* getOwnerPlugin() {
			return _owner;
		}

		virtual tNet getNet() {
			return this->_net;
		}

		virtual enIMessageType getType() {
			return this->_type;
		}

		virtual Stamina::Version getVersion() {
			return this->_version;
		}

		virtual const String& getSig() {
			return this->_sig;
		}

		virtual const String& getName() {
			return this->_name;
		}

		virtual const String& getNetName() {
			return this->_netName;
		}

		virtual enPluginPriority getPriority() {
			return this->_priority;
		}

		virtual bool canHotPlug() {
			return false;
		}

		virtual bool canPlugOut() {
			return this->canHotPlug() || Konnekt::running == false;
		}

		virtual int IMessageDirect(cCtrl* sender, sIMessage_base* im) {
			im->sender = sender->ID();
			return this->sendIMessage(im);
		}

		/** Wysy³a wiadomoœæ BEZPOŒREDNIO! */
		int sendIMessage(sIMessage_base* im);

		virtual bool plugOut(cCtrl* sender, const StringRef& reason, bool quiet, enPlugOutUnload unload);

		COLORREF getDebugColor() {
			return _debugColor;
		}

		cCtrl_* getCtrl() {
			return _ctrl;
		}

		virtual bool isRunning() {
			return _running;
		}

		void madeError(const StringRef& msg , unsigned int severity);

		tPluginId getId() {
			return _id;
		}


	private:

		int callIMessageProc(sIMessage_base*im);


		void initClassic(const StringRef& file, void* imessageProc = 0);
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
			return *_list[i];
		}

		bool exists(tPluginId i) {
			i = (tPluginId) this->getIndex(i);
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

		Plugin* findName(const StringRef& name);

		Plugin* findSig(const StringRef& sig);

		String getName(tPluginId id);

		void sortPlugins();

		void cleanUp();


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