#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include <Stamina\ObjectImpl.h>
// Info o wtyczce


namespace Konnekt {
	
	class cPlug {
	public:
		fIMessageProc IMessageProc;
		HMODULE hModule;
		int net;
		int type;
		string version;
		unsigned int versionNumber;
		string sig;
		string core_v;
		string ui_v;
		string name;
		string netname;
		string file;
		COLORREF debugColor;
		bool running;
		unsigned int ID;
		class cCtrl_ * Ctrl;
		int IMessage(unsigned int id , int p1=0 , int p2=0 , unsigned int sender=0);
		void madeError(const CStdString msg , unsigned int severity);
		const char * GetName();
		cPlug();
		PLUGP_enum priority;
	};

	class cPlugs {
	public:
		deque <cPlug> Plug;
		typedef deque <cPlug>::iterator Plug_it_t;

		cPlug & operator [] (int i) {
			if (i >= PLUG_MAX_COUNT) // mamy ID
				i = FindID(i);
			return Plug[i];}
		bool exists(int i) {
			if (i >= PLUG_MAX_COUNT) // mamy ID
				i = FindID(i);
			return (i >= 0 && i < Plug.size());
		}
		int size() {return Plug.size();}
		int push(cPlug & v) {Plug.push_back(v);return Plug.size()-1;}
		int PlugIN(const char * file , const char * cert = 0 , int ex=0);
		int PlugOUT(int nr);
		void sort(void);

		const char * Name (unsigned int);

		int FindID (unsigned int id , int start=0);

		int Find (int net , unsigned int type , int start=0);

	};

	extern cPlugs Plug;

	void setPlugins(bool noDlg = false , bool startUp = true);
	void checkVersions(void);


	cCtrl_ * createPluginCtrl();
	cCtrl_ * createCorePluginCtrl();

	void PlugsDialog(bool firstTimer = false);

};