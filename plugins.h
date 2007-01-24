#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include <Stamina\ObjectImpl.h>
#include <Stamina\Logger.h>
// Info o wtyczce

using namespace Stamina;

namespace Konnekt {

	namespace PLG {
		const DT::tColId file = 0;
		const DT::tColId md5 = 1;
		const DT::tColId sig = 2;
		const DT::tColId load = 3;
		const DT::tColId isNew = 4;
	};

	extern tTableId tablePlugins;

	void pluginsInit();

	class Controler_;

	class Plugin: public LockableObject<iPlugin> {
	public:

		friend class Plugins;

		STAMINA_OBJECT_CLASS(Konnekt::Plugin, Konnekt::iPlugin);

		Plugin(tPluginId pluginId = pluginNotFound);

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

		int IMessageDirect(tIMid id, int p1, int p2) {
			return this->IMessageDirect(Ctrl, &sIMessage_2params(id, p1, p2));
		}

		virtual int IMessageDirect(Controler* sender, sIMessage_base* im) {
			if (sender) {
				im->sender = sender->ID();
			} else {
				im->sender = pluginNotFound;
			}
			return this->sendIMessage(im);
		}

		/** Wysy³a wiadomoœæ BEZPOŒREDNIO! */
		int sendIMessage(sIMessage_base* im);

		virtual bool plugOut(Controler* sender, const StringRef& reason, bool quiet, enPlugOutUnload unload);

		virtual bool subclassIMessageProc(Controler* sender, void*& proc, void*& object);

		virtual oPlugin getLastSubclasser() {
			return _subclasser;
		}

		virtual void resetSubclassing() {
			_subclasser.reset();
			_imessageProc = _originalProc;
			_imessageObject = _imessageObject;
		}

		COLORREF getDebugColor() {
			return _debugColor;
		}

		Controler_* getControler() {
			return _ctrl;
		}
		Controler_* getCtrl() {
			return _ctrl;
		}

		virtual bool isRunning() {
			return _running;
		}

		void madeError(const StringRef& msg , unsigned int severity);

		tPluginId getId() {
			return _id;
		}

		const oLogger& getLogger() {
			return _logger;
		}

		

	private:

		int callIMessageProc(sIMessage_base*im) {
			return iPlugin::callIMessageProc(im, _imessageProc, _imessageObject);
		}


		void initClassic(const StringRef& file, void* imessageProc = 0);
		void initVirtual(Plugin& owner, void* imessageObject, void* imessageProc);
		void initData();
		void run();
		void checkApiVersions(bool registerVersions);

		void deinitialize();

	private:

		void* _imessageProc;
		void* _imessageObject;
		void* _originalProc;
		void* _originalObject;
		oPlugin _subclasser;
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

		class Controler_ * _ctrl;
		oLogger _logger;
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
			i = this->getIndex((tPluginId)i);
			if (i == pluginNotFound) {
				throw Stamina::ExceptionString("Plugin out of range");
			}
			return *_list[i];
		}

		bool exists(tPluginId i) {
			i = (tPluginId) this->getIndex(i);
			return (i != pluginNotFound && (signed)i >= 0 && (unsigned)i < _list.size());
		}

		int count() {
			return _list.size();
		}

		tList::iterator begin() {
			return _list.begin();
		}

		tList::iterator end() {
			return _list.end();
		}

		tList::reverse_iterator rbegin() {
			return _list.rbegin();
		}

		tList::reverse_iterator rend() {
			return _list.rend();
		}

		enum enPlugInResult {
			resultPlugged = 2,
			resultFailedRetry = 1,
			resultFailedNoRetry = 0,
		};

		void plugInClassic(const StringRef& filename, void* imessageProc = 0, tPluginId pluginId = pluginNotFound) throw(...);

		void plugInVirtual(Plugin& owner, void* object, void* proc, tPluginId pluginId = pluginNotFound) throw(...);

		bool plugOut(Plugin& plugin, bool removeFromList);

		void cleanup();

		int findPlugin(tNet net , enIMessageType type , unsigned int start=0);

		Plugin* findName(const StringRef& name);

		Plugin* findSig(const StringRef& sig);

		String getName(tPluginId id);

		String getSig(tPluginId id);

		void sortPlugins();

		void cleanUp();


		static void setPlugins(bool noDlg = false , bool startUp = true);
		static void checkVersions(void);

	private:

		tList _list;

	};

	extern Plugins plugins;

	extern VersionControl apiVersions;




	Controler_ * createPluginCtrl(Plugin& plugin);
	Controler_ * createCorePluginCtrl(Plugin& plugin);

	void PlugsDialog(bool firstTimer = false);

};