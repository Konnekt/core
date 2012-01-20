#include "stdafx.h"
#include "plugins.h"
#include "imessage.h"
#include "resources.h"
#include "plugins_ctrl.h"
#include "tables.h"
#include "tools.h"
#include "profiles.h"
#include "argv.h"

namespace Konnekt {

	cPlugs Plug;

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


	const char * cPlugs::Name (unsigned int h) {
		if (!h) return "CORE";
		if (h<3) return _sprintf("___BC%u",h-1);
		int pos=FindID(h);
		if (pos>=0) return Plug[pos].name.c_str();
		else return _sprintf("%#.4x",h);
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
				err +=inttoch(errNo);
				err +="] ";
				err +=GetLastErrorMsg(errNo);
				strcpy(TLS().buff , err.c_str());
				throw (char*)TLS().buff;
			}
			dll.IMessageProc=(fIMessageProc)GetProcAddress(hModule , "IMessageProc");
			if (!dll.IMessageProc) {
				dll.IMessageProc=(fIMessageProc)GetProcAddress(hModule , "_IMessageProc@4");
				if (!dll.IMessageProc)
					throw "IMessageProc not found!";
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
			dll.version=safeChar(IMessageDirect(dll,IM_PLUG_VERSION , 0 , 0 , 0));
			if (dll.version.empty()) {
				FileVersionInfo(dll.file.c_str() , TLS().buff2);
				dll.version = TLS().buff2;
			}
			// Zamieniamy wersje tekstowa na liczbowa...
			Stamina::RegEx reg;
			if (reg.match("/(\\d*)[., ]*(\\d*)[., ]*(\\d*)[., ]*(\\d*)/" , dll.version.c_str()))
				dll.versionNumber = VERSION_TO_NUM(atoi(reg[1].c_str()) , atoi(reg[2].c_str()) , atoi(reg[3].c_str()) , atoi(reg[4].c_str()));
			else
				dll.versionNumber = 0;
			dll.sig=safeChar(IMessageDirect(dll,IM_PLUG_SIG , 0 , 0 , 0));
			//    dll.core_v=safeChar(IMessageDirect(dll,IM_PLUG_CORE_V , 0 , 0 , 0));
			//    dll.ui_v=safeChar(IMessageDirect(dll,IM_PLUG_UI_V , 0 , 0 , 0));

			dll.net=IMessageDirect(dll,IM_PLUG_NET , 0 , 0);
			dll.name=safeChar(IMessageDirect(dll,IM_PLUG_NAME , 0 , 0 , 0));
			dll.netname=safeChar(IMessageDirect(dll,IM_PLUG_NETNAME , 0 , 0 , 0));

			if (IMessageDirect(dll,IM_PLUG_SDKVERSION , 0 , 0 , 0) < KONNEKT_SDK_V) throw "Wtyczka przygotowana zosta³a dla starszej wersji API. Poszukaj nowszej wersji!";

			if (Plug.size() > 0 && dll.net==0) throw "Bad Net value!";
			if (dll.sig=="" || dll.name=="") throw "Sig|Name required!";
			//    if (dll.core_v!="" && dll.core_v!=core_v) throw "incompatible Core";
			//    if (ui_v!="" && dll.ui_v!="" && dll.ui_v!=ui_v) throw "incompatible UserInterface";
			if (Plug.size()==0) {//ui_v=dll.sig;
			ui_sender=dll.ID;
			}
			dll.Ctrl = ex? createCorePluginCtrl() : createPluginCtrl();
			dll.Ctrl->setCtrl(dll.ID , hModule);
			dll.running = true;
			if (!IMessageDirect(dll,IM_PLUG_INIT , (tIMP)(dll.Ctrl) , (tIMP)dll.hModule , 0))
				throw "IM_INIT failed";
			if (ex) IMessageDirect(dll,IM_PLUG_INITEX , (tIMP)(dll.Ctrl) , 0 , 0);

			int pos = this->push(dll);

			if (dll.net == -1) { /* odpytujemy raz jeszcze, sam nas o to prosi³... */
				Plug[pos].net =IMessageDirect(dll,IM_PLUG_NET , 0 , 0);
				Plug[pos].name=safeChar(IMessageDirect(dll,IM_PLUG_NAME , 0 , 0 , 0));
				Plug[pos].netname=safeChar(IMessageDirect(dll,IM_PLUG_NETNAME , 0 , 0 , 0));
				if (Plug[pos].name=="") throw "Sig|Name required!";
			}
			// Wtyczka zostala zatwierdzona i dodana

			//    cPlug & plg = Plug[Plug.size()-1];


		} catch (char * er) {
			if (dll.Ctrl) {delete dll.Ctrl;dll.Ctrl = 0;}
			if (hModule) FreeLibrary(hModule);
			CStdString errMsg;
			errMsg.Format(resString(IDS_ERR_DLL).c_str() , file , er);
			int ret = MessageBox(NULL , errMsg , resStr(IDS_APPNAME) , MB_ICONWARNING|MB_YESNOCANCEL|MB_TASKMODAL);
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
		string ver = Cfg.getch(0 , CFG_VERSIONS);
		unsigned int prevVer = findVersion(ver , "CORE" , versionNumber);
		if (versionNumber > prevVer) updateCore(prevVer);
		for (int i = 0; i<Plug.size(); i++) {
			prevVer = findVersion(ver , CStdString(Plug[i].file).ToLower() , Plug[i].versionNumber);
			if (Plug[i].versionNumber > prevVer) {Plug[i].IMessage(IM_PLUG_UPDATE , prevVer);}
		}
		Cfg.setch(0 , CFG_VERSIONS , ver.c_str());
	}



	void setPlugins(bool noDlg , bool startUp) {
		string plugDir = resStr(IDS_PLUGINDIR);
		string dir = plugDir + "*.dll";
		WIN32_FIND_DATA fd;
		HANDLE hFile;
		BOOL found;
		found = ((hFile = FindFirstFile(dir.c_str(),&fd))!=INVALID_HANDLE_VALUE);
		int i = 0;

		bool newOne = false;
		while (found)
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				&& stricmp("ui.dll" , fd.cFileName)
				) {
					int p = Plg.findby((TdEntry)string(plugDir + fd.cFileName).c_str() , PLG_FILE);
					if (p < 0) {
						newOne = true;
						p = Plg.addrow();
						Plg.setint(p , PLG_NEW , 1);
					} else Plg.setint(p , PLG_NEW , 0);
					Plg.setch(p , PLG_FILE , (plugDir + fd.cFileName).c_str());
					i++;
				}
				found = FindNextFile(hFile , &fd);
		}
		FindClose(hFile);   
		if ((startUp && getArgV(ARGV_PLUGINS , false)) || (newOne && !noDlg))
			PlugsDialog(Konnekt::newProfile);
		if (!startUp) return;
		newOne = false;
		// Wlaczanie wtyczek:
		for (unsigned int i = 0 ; i < Plg.getrowcount(); i++) {
			if (Plg.getint(i , PLG_NEW)==-1) {Plg.deleterow(i);i--;continue;} // usuwa nie istniejaca wtyczke
			if (Plg.getint(i , PLG_LOAD)!=1) continue;
			if (!Plug.PlugIN(Plg.getch(i , PLG_FILE)))
			{Plg.setint(i , PLG_LOAD , -1);
			newOne = 1;
			//       MessageBox(NULL ,resStr(IDS_ERR_PLUGERROR),STR(resStr(IDS_APPNAME)),MB_ICONWARNING|MB_OK|MB_TASKMODAL);exit(0);
			}

		}
		Tables::savePlg();
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
			int pl = Plg.findby((TdEntry)this->file.c_str() , PLG_FILE);
			if (pl < 0) return;
			Plg.setint(pl , PLG_LOAD , -1);
			Tables::savePlg();
			if (!running) {
				deInit();
			} else {
				deInit();
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
				//cPlug temp2 = *it;
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