#include "stdafx.h"
#include "profiles.h"
#include "konnekt_sdk.h"
#include "tables.h"
#include "main.h"
#include "resources.h"
#include "profiles.h"
#include "windows.h"


namespace Konnekt {
	CStdString profile;
	CStdString profileDir;
	CStdString profilesDir;
	bool newProfile = false;
	unsigned char md5digest [16];

	void profilePass () {
		sDIALOG_access sd;
		if (md5digest[0]) {
			sd.title = "Konnekt - zmiana has³a";
			sd.info = "Podaj aktualne has³o profilu";
			if (!IMessage(IMI_DLGPASS , 0 , 0 , (int)&sd , 0)) return;
			//    IMLOG("[%s]==[%s]==[%s]" , sd.ch , MD5(sd.ch) , md5digest);
			if (memcmp(MD5(sd.pass , (unsigned char *) TLS().buff) , md5digest , 16)) {
				IMessage(IMI_ERROR , 0 , 0 , (int)"Poda³eœ z³e has³o!");
				return;
			}
		}
		sd.pass = "";
		sd.save = 0;
		sd.title = "Konnekt - zmiana has³a";
		sd.info = "Podaj nowe has³o dla profilu";
		if (!IMessage(IMI_DLGSETPASS , 0 , 0 , (int)&sd , 0)) return;
		if (!*sd.pass) memset(md5digest , 0 , 16);
		else MD5(sd.pass , md5digest);

		IMDEBUG(DBG_WARN, "Has³o profilu zosta³o zmienione.");
		// rozsy³amy informacjê
		IMessage(IM_PASSWORDCHANGED, NET_BROADCAST, IMT_ALL, 0, 0);

		Cfg.changed = Plg.changed = Msg.changed = Cnt.changed = true;
		Tables::tables.saveAll();
		Tables::saveCfg();
		Tables::saveCnt();
		Tables::savePlg();
		Tables::saveMsg();

	}
	void setProfilesDir(void) {
		sDIALOG_choose sdc;
		sdc.title="Katalog profili";
		sdc.info="Wybierz w jakim katalogu bêd¹ przechowywane profile";
		sdc.items="profil u¿ytkownika\nkat. Konnekta";
		sdc.width=150;
		sdc.def = 1;
		switch (IMessage(IMI_DLGBUTTONS , 0 , 0 , (int)&sdc)) {
		case 2:  profilesDir = "profiles\\";break;
		default: {
			char buff [MAX_PATH];
			buff[0]=0;
			SHGetSpecialFolderPath(0,buff , CSIDL_APPDATA , true);
			profilesDir=buff+string("\\stamina\\konnekt\\");
			break;}

		}
	}
	int setProfile(bool load , bool check) {
		//     char * error=0;
		//     error[13]=0;
		CdtFileBin FBin;
		
		memset(FBin.md5digest , 0 , 16);

		char * ch = strdup(appPath);
		filepath(ch);
		profileDir = ((profilesDir[1]!=':' && profilesDir[1]!='\\')?string(ch):"") + string(profilesDir);
		free(ch);
		// Sprawdzamy czy katalog jest pe³ny, czy pusty...
		if (profile=="") {
			WIN32_FIND_DATA fd;
			HANDLE hFile;
			if ((hFile = FindFirstFile(profilesDir + "*.*",&fd))!=INVALID_HANDLE_VALUE) {
				do {
					if (*fd.cFileName != '.' && fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						profile = "?";
						break;
					}
				} while (FindNextFile(hFile , &fd));
				FindClose(hFile);
			}
		}	

		if (profile=="") {
			unsigned long i=MAX_STRING;
			GetUserName(TLS().buff , &i);
			profile=TLS().buff;
			if (profile.empty()) profile="podstawowy";
			sDIALOG_enter sde;
			sde.title="Nowy profil";
			sde.info="Zostanie za³o¿ony nowy profil. Podaj jego nazwê.";
			sde.value=(char*)profile.c_str();
			IMessage(IMI_DLGENTER , 0 , 0 , (int)&sde);
			if (*sde.value) {
				profile = sde.value;
				check = false;
			}
		}
		if (profile=="?" || (check && GetFileAttributes((profileDir + profile).c_str()) == INVALID_FILE_ATTRIBUTES)) {
			if (profile == "?") profile="";
			return DialogBox(hInst, MAKEINTRESOURCE(IDD_PROFILE), 0, ProfileDialogProc);
		}

		profileDir+= profile + "\\";
		if (access(profileDir.c_str() , 0)) {
			mkdirs(profileDir.c_str());

			//      UIMessage(IMI_WARNING , (int)STR(_sprintf(resStr(IDS_INF_NEWPROFILE),profile.c_str())) , 0);
		}
		if (access((profileDir + "cfg.dtb").c_str() , 0))
			newProfile = true;



		string fn = string(profileDir+"cfg.dtb");
		try {
			FBin.assign(&Cfg);
			if (FBin.open(fn.c_str() , DT_READ)) throw 0;
			if (FBin.freaddesc()) throw 1;
			// Je¿eli aktualny digest jest inny od tego w pliku, trzeba zapytaæ o nowy...
			if (memcmp(md5digest , FBin.md5digest , 16)) {
				if (md5digest[0]) // has³o by³o ustawiane - trzeba wrzuciæ monit o z³ym haœle...
					IMessage(IMI_ERROR , 0 , 0 , (int)resStr(IDS_ERR_BADPASSWORD));
				memcpy(md5digest , FBin.md5digest , 16);
				int r=0;
				if ((r=DialogBox(hInst, MAKEINTRESOURCE(IDD_PROFILE), 0, ProfileDialogProc))!=IDCANCEL) return r;
			} else
				memcpy(md5digest , FBin.md5digest , 16);
			// Inaczej uzupe³nimy kolumny...
			//Cfg.cols=FBin.fcols;
			FBin.close();
		} catch (int e)
		{FBin.close();
		if (!e) {
			UIMessage(IMC_LOG , (int)STR(_sprintf(resStr(IDS_INF_FILESKIPPED),fn.c_str())),0);
		}
		else {UIMessage(IMI_WARNING , (int)STR(_sprintf(resStr(IDS_INF_BADFILE),fn.c_str())),0);exit(1);}
		}


		fn = string(profileDir+"cnt.dtb");
		try {
			FBin.assign(&Cnt);
			if (FBin.open(fn.c_str() , DT_READ)) throw 0;
			if (FBin.freaddesc()) throw 1;
			if (memcmp(FBin.md5digest , md5digest , 16)
				/*|| (Cnt.dbID && (Cnt.dbID != dbID))*/)
			{IMessage(IMI_ERROR , 0 , 0 , (int)STR(_sprintf(resStr(IDS_ERR_UNAUTHORIZED) , "cnt.dtb")));
			gracefullExit();}
			//      Cnt.cols=FBin.fcols;
			FBin.close();
		} catch (int e)
		{FBin.close();
		if (!e) UIMessage(IMC_LOG , (int)STR(_sprintf(resStr(IDS_INF_FILESKIPPED),fn.c_str())),0);
		else {UIMessage(IMI_WARNING , (int)STR(_sprintf(resStr(IDS_INF_BADFILE),fn.c_str())),0);gracefullExit();}
		}

		FBin.assign(&Plg);
		if (!FBin.load(string(profileDir+"plg.dtb").c_str()))
		{/*if (Plg.dbID && (Plg.dbID != dbID))
		 {IMessage(IMI_ERROR , 0 , 0 , (int)STR(_sprintf(resStr(IDS_ERR_UNAUTHORIZED) , "plg.dtb")));
		 exit(0);}*/
		}
		//    IMLOG("PLG loaded %d" , Plg.getrowcount());  
		return 1;
	}

	// Oczyszcza tablice po zaladowaniu z niepotrzebnych smieci...
	void prepareProfile() {
		// CNT
		Cnt.lock(DT_LOCKALL);
		for (unsigned int i = 1; i < Cnt.getrowcount() ; i++) {
			int s = Cnt.getint(i , CNT_STATUS);
			// ST_OFFLINE
			s &= ~(0xFF);
			if (s & ST_NOTINLIST) {
				Cnt.deleterow(i--);
				continue;
			}
			Cnt.setint(i , CNT_STATUS , s | ST_OFFLINE);
		}
		Cnt.unlock(DT_LOCKALL);

		// MSG
		Msg.lock(DT_LOCKALL);
		for (unsigned int i = 0; i < Msg.getrowcount() ; i++) {
			if (Msg.getint(i , MSG_FLAG) & MF_NOSAVE) {
				Msg.deleterow(i--);
				continue;
			}
			// Usuwamy zbêdne flagi
			Msg.setint(i , MSG_FLAG , 
				Msg.getint(i , MSG_FLAG) & (
				~(MF_PROCESSING|MF_OPENED)
				));
		}
		Msg.unlock(DT_LOCKALL);
	}


	int loadProfile(void) {
		CdtFileBin FBin;
		Tables::savePrepare(FBin);
		FBin.assign(&Cfg);
		FBin.loadAll(string(profileDir+"cfg.dtb").c_str());
		FBin.assign(&Cnt);
		FBin.loadAll(string(profileDir+"cnt.dtb").c_str());
		FBin.assign(&Msg);
		if (!FBin.loadAll(string(profileDir+"msg.dtb").c_str())) {
			if (memcmp(FBin.md5digest , md5digest , 16)
				/*|| (Msg.dbID && (Msg.dbID != dbID))*/)
			{IMessage(IMI_ERROR , 0 , 0 , (int)STR(_sprintf(resStr(IDS_ERR_UNAUTHORIZED) , "msg.dtb")));
			gracefullExit();}
		}

		//Cfg.dbID = Cnt.dbID = Msg.dbID = Plg.dbID = dbID;

		if (!Cnt.getrowcount()) {Cnt.addrow();}
		if (!Cfg.getrowcount()) {Cfg.addrow();}
		prepareProfile();
		IMessage(IM_CFG_LOAD , -1, IMT_CONFIG|IMT_CONTACT , 0 ,0,0);
		return 0;
	}

};
