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

	cPlugs Plug;

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


	int cPlugs::FindID (unsigned int id , int start) {
		if (id < PLUG_MAX_COUNT) // mamy pozycjê...
			return id;
		for (Plug_it_t Plug_it=Plug.begin()+start; Plug_it!=Plug.end(); Plug_it++)
			if (Plug_it->ID==id) return Plug_it-Plug.begin();
		return -1;
	}




	int cPlugs::Find (int net , unsigned int type , int start) {
		for (Plug_it_t Plug_it=Plug.begin()+start; Plug_it!=Plug.end(); Plug_it++)
			if ((type==-1 || (Plug_it->type & type)==type) && (net==-1 || net==-2 || Plug_it->net==net) ) return Plug_it-Plug.begin();
		return -1;
	}


	std::string cPlugs::Name (unsigned int h) {
		if (!h) return "CORE";
		if (h<3) return stringf("___BC%u",h-1);
		int pos=FindID(h);
		if (pos>=0) return Plug[pos].name;
		else return stringf("%#.4x",h);
	}
	int cPlugs::PlugIN(const char * file , const char * cert , int ex) {
		HMODULE hModule;
		cPlug dll;
		IMLOG("* plugin - %s" , file);
		try {
			dll.priority = PLUGP_NONE;
			dll.Ctrl = 0;
			hModule = LoadLibrary(file);

			if (!hModule) {
				string err = "Load Failed [";
				int errNo = GetLastError();
				err += inttostr(errNo);
				err += "] ";
				err += getErrorMsg(errNo);
				//strcpy(TLS().buff , err.c_str());
				throw ExceptionString(err);
			}
			dll.IMessageProc=(fIMessageProc)GetProcAddress(hModule , "IMessageProc");
			if (!dll.IMessageProc) {
				dll.IMessageProc=(fIMessageProc)GetProcAddress(hModule , "_IMessageProc@4");
				if (!dll.IMessageProc)
					throw ExceptionString("IMessageProc not found!");
			}

			dll.file = file;
			dll.hModule=hModule;
			//	if (Plug.size() < 1) { // ID dla UI jest inne od pozosta³ych
			//		dll.ID = Plug.size(); 
			//	} else {
			dll.ID=(unsigned int)Plug.size()+PLUG_MAX_COUNT; // <=0x80 to identyfikatory... Do 127 wtyczek!
			//	}
			/*    if (cert) {
			char * ch = (char*)IMessageDirect(dll,IM_PLUG_CERT , 0 , 0);
			if (!ch || strcmp(cert , ch)) throw "Unsuplied CERT";
			}*/
			dll.running = true;
			dll.type=IMessageDirect(dll,IM_PLUG_TYPE , 0 , 0 , 0);
			dll.version = Version( safeChar(IMessageDirect(dll,IM_PLUG_VERSION , 0 , 0 , 0)) );
			if (dll.version.empty()) {
				FileVersion fv(dll.file);
				dll.version = fv.getFileVersion();
			}
			dll.sig=safeChar(IMessageDirect(dll,IM_PLUG_SIG , 0 , 0 , 0));
			//    dll.core_v=safeChar(IMessageDirect(dll,IM_PLUG_CORE_V , 0 , 0 , 0));
			//    dll.ui_v=safeChar(IMessageDirect(dll,IM_PLUG_UI_V , 0 , 0 , 0));

			dll.net=IMessageDirect(dll,IM_PLUG_NET , 0 , 0);
			dll.name=safeChar(IMessageDirect(dll,IM_PLUG_NAME , 0 , 0 , 0));
			dll.netname=safeChar(IMessageDirect(dll,IM_PLUG_NETNAME , 0 , 0 , 0));

			if (IMessageDirect(dll,IM_PLUG_SDKVERSION , 0 , 0 , 0) < KONNEKT_SDK_V) throw ExceptionString("Wtyczka przygotowana zosta³a dla starszej wersji API. Poszukaj nowszej wersji!");

			if (Plug.size() > 0 && dll.net==0) throw ExceptionString("Bad Net value!");
			if (dll.sig=="" || dll.name=="") throw ExceptionString("Sig|Name required!");
			//    if (dll.core_v!="" && dll.core_v!=core_v) throw "incompatible Core";
			//    if (ui_v!="" && dll.ui_v!="" && dll.ui_v!=ui_v) throw "incompatible UserInterface";
			if (Plug.size()==0) {//ui_v=dll.sig;
			ui_sender=dll.ID;
			}
			dll.Ctrl = ex? createCorePluginCtrl() : createPluginCtrl();
			dll.Ctrl->setCtrl(dll.ID , hModule);
			dll.running = true;
			if (!IMessageDirect(dll,IM_PLUG_INIT , (tIMP)(dll.Ctrl) , (tIMP)dll.hModule , 0))
				throw ExceptionString("IM_INIT failed");
			if (ex) IMessageDirect(dll,IM_PLUG_INITEX , (tIMP)(dll.Ctrl) , 0 , 0);

			int pos = this->push(dll);

			if (dll.net == -1) { /* odpytujemy raz jeszcze, sam nas o to prosi³... */
				Plug[pos].net =IMessageDirect(dll,IM_PLUG_NET , 0 , 0);
				Plug[pos].name=safeChar(IMessageDirect(dll,IM_PLUG_NAME , 0 , 0 , 0));
				Plug[pos].netname=safeChar(IMessageDirect(dll,IM_PLUG_NETNAME , 0 , 0 , 0));
				if (Plug[pos].name=="") throw ExceptionString("Sig|Name required!");
			}
			// Wtyczka zostala zatwierdzona i dodana

			//    cPlug & plg = Plug[Plug.size()-1];


		} catch (Exception& e) {
			if (dll.Ctrl) {delete dll.Ctrl;dll.Ctrl = 0;}
			if (hModule) FreeLibrary(hModule);
			CStdString errMsg;
			errMsg.Format(loadString(IDS_ERR_DLL).c_str() , file , e.getReason().c_str());
			int ret = MessageBox(NULL , errMsg , loadString(IDS_APPNAME).c_str() , MB_ICONWARNING|MB_YESNOCANCEL|MB_TASKMODAL);
			if (ret == IDCANCEL) abort();
			return ret == IDYES? 0 : -1;
		}
		return 1;
	}


	int cPlugs::PlugOUT(int nr) {
		//  if (Plug[nr].type & IMT_PROTOCOL)
		IMessage(IM_PLUG_PLUGOUT , NET_BROADCAST , IMT_ALL , Plug[nr].ID , 0 );
		bool nofree = IMessageDirect(Plug[nr], IM_PLUG_DONTFREELIBRARY, 0, 0, 0);
		Plug[nr].running=false;
		IMessageDirect(Plug[nr],IM_PLUG_DEINIT , 0 , 0 , 0);
		if (!nofree)
			FreeLibrary((HMODULE)Plug[nr].hModule);
		delete Plug[nr].Ctrl;
		Plug[nr].Ctrl = 0;
		return 0;
	}
	void checkVersions(void) {
		string ver = Tables::cfg->getString(0 , CFG_VERSIONS);
		unsigned int prevVer = findVersion(ver , "CORE" , versionNumber);
		if (versionNumber > prevVer) updateCore(prevVer);
		for (int i = 0; i<Plug.size(); i++) {
			prevVer = findVersion(ver , CStdString(Plug[i].file).ToLower() , Plug[i].version.getInt());
			if (Plug[i].version.getInt() > prevVer) {Plug[i].IMessage(IM_PLUG_UPDATE , prevVer);}
		}
		Tables::cfg->setString(0 , CFG_VERSIONS , ver);
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
				int p = plg->findRow(0, DT::Find::EqStr(PLG::file, plugDir + ff.found().getFileName()));
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
			if (!Plug.PlugIN(plg->getString(i , PLG::file).c_str())) {
				plg->setInt(i , PLG::load , -1);
				newOne = 1;
			}
		}
        plg->save();
		IMLOG("- Plug.sort");
		Plug.sort();
		//  if (newOne)
		//    IMessageDirect(Plug[0] , IMI_DLGPLUGS , 2);


	}
	const char * cPlug::GetName() {
		return (const char*)this->IMessage(IM_PLUG_NAME);
	}


	void cPlug::madeError(const CStdString msg , unsigned int severity) {
		CStdString info = "Wtyczka \""+this->name+"\" zrobi³a bubu:\r\n" + string(msg) + "\r\n\r\nPrzejmujemy siê tym?";
		if (MessageBox(0 , info , "Konnekt" , MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_YESNO) == IDYES)
		{ 
			Tables::oTableImpl plg(tablePlugins);
			int pl = plg->findRow(0, DT::Find::EqStr(PLG::file, this->file));
			if (pl == DT::rowNotFound) return;
			plg->setInt(pl , PLG::load , -1);
			plg->save();
			if (!running) {
				deinitialize();
			} else {
				deinitialize();
				restart();
			}
		}
	}


	// ---------------------------------------------

	void cPlugs::sort(void) {
		// Na poczatku wszystkie wtyczki maja 0 dziêki czemu bêdziemy je
		// trzymaæ na samym koñcu listy przez ca³y czas...
		Plug_it_t it = Plug.begin()+1;
		while (it != Plug.end()) {
			// sprawdzamy priorytety
			it->priority = (PLUGP_enum)it->IMessage(IM_PLUG_PRIORITY , 0 , 0);
			if (!it->priority || it->priority > PLUGP_HIGHEST) it->priority=PLUGP_STANDARD;
			// szukamy teraz, gdzie go wrzucic... na pewno gdzies wczesniej
			Plug_it_t ins = Plug.begin()+1;
			while (ins != it && ins->priority >= it->priority) {ins++;}
			if (ins!=it) {
				int pos = it - Plug.begin();
				Plug.insert(ins , *it);
				it = Plug.erase(Plug.begin() + pos + 1);
				//            it++; // na pewno wrzucamy wczesniej, wiec sie troche przesunelo...
				cPlug temp2 = *it;
				continue;
			}
			it ++;
		}
	}



	int cPlug::IMessage(unsigned int id , int p1 , int p2 , unsigned int sender) {
		return IMessageDirect(*this , id,p1,p2,sender);
	}


	cPlug::cPlug() {
		this->debugColor = getUniqueColor(Plug.size(), 4, 0xFF, true, true);
	}

};