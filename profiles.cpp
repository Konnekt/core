#include "stdafx.h"
#include "profiles.h"
#include "konnekt_sdk.h"
#include "tables.h"
#include "main.h"
#include "resources.h"
#include "profiles.h"
#include "windows.h"
#include "argv.h"

using namespace Stamina;

namespace Konnekt {
	CStdString profile;
	CStdString profileDir;
	CStdString profilesDir;
	bool newProfile = false;

	MD5Digest passwordDigest;

	void initializeProfileDirectory() {
		if (getArgV(ARGV_PROFILESDIR) && !getArgV(ARGV_PROFILESDIR , true , 0)) {
			if (profilesDir.empty()) {
				MessageBox(0 , "Katalog profili jeszcze nie zosta³ zdefiniowany!" , "Konnekt" , MB_ICONWARNING);
			} else {
				MessageBox(0 , ("Profile mieszcz¹ siê w katalogu: \n\"" + unifyPath(profilesDir, false) +"\"").c_str(), "Konnekt" , MB_ICONINFORMATION);
				ShellExecute(0 , "Explore" , unifyPath(profilesDir, false).c_str() , "" , "" , SW_SHOW);				
			}
			gracefullExit();
		}
	}


	void profilePass () {
		sDIALOG_access sd;
		if (passwordDigest.empty() == false) {
			sd.title = "Konnekt - zmiana has³a";
			sd.info = "Podaj aktualne has³o profilu";
			if (!IMessage(IMI_DLGPASS , 0 , 0 , (int)&sd , 0)) return;
			//    IMLOG("[%s]==[%s]==[%s]" , sd.ch , MD5(sd.ch) , md5digest);
			if (MD5Digest(sd.pass) != passwordDigest) {
				IMessage(IMI_ERROR , 0 , 0 , (int)"Poda³eœ z³e has³o!");
				return;
			}
		}
		sd.pass = "";
		sd.save = 0;
		sd.title = "Konnekt - zmiana has³a";
		sd.info = "Podaj nowe has³o dla profilu";
		if (!IMessage(IMI_DLGSETPASS , 0 , 0 , (int)&sd , 0)) return;
		if (!*sd.pass) { // resetujemy has³o
			passwordDigest.reset();
		} else {
			passwordDigest.calculate(sd.pass);
		}

		IMDEBUG(DBG_WARN, "Has³o profilu zosta³o zmienione.");
		// rozsy³amy informacjê
		IMessage(IM_PASSWORDCHANGED, NET_BROADCAST, IMT_ALL, 0, 0);

		Tables::tables.setProfilePassword(passwordDigest);

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

		profileDir = Stamina::unifyPath(( profilesDir[1] != ':' && profilesDir[1] != '\\' ) ? getFileDirectory(appPath) : "", true) + string(profilesDir);

		// Sprawdzamy czy w katalogu profili s¹ ju¿ jakieœ katalogi i jak s¹, to w³¹czamy wybór profilu...
		if (profile == "") {
			Stamina::FindFile ff(unifyPath(profilesDir, true) + "*.*");
			ff.setDirOnly();
			ff.excludeAttribute(Stamina::FindFile::attHidden);
			ff.excludeAttribute(Stamina::FindFile::attSystem);
			if (ff.find()) {
				profile = "?";
			}
		}

		if (profile == "") {
			// pobieramy nazwê domyœlnego profilu...
			unsigned long i = 255;
			GetUserName(stringBuffer(profile, i), &i);
			stringRelease(profile, i);

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

		profileDir += profile + "\\";

		if (profile=="?" || (check && fileExists(profileDir) == false)) {
			// je¿eli mamy wyœwietliæ okno profili, lub wybrany wczeœniej profil nie istnieje - pokazujemy okno wyboru profili...
			if (profile == "?") profile = "";
			return DialogBox(Stamina::getHInstance(), MAKEINTRESOURCE(IDD_PROFILE), 0, ProfileDialogProc);
		}

		if (fileExists(profileDir) == false) {
			Stamina::createDirectories(profileDir);
		}

		if ( Stamina::fileExists(profileDir + "cfg.dtb") == false ) {
			newProfile = true;
		}

		DT::FileBin fb;
		// sprawdzamy czy jest has³o...
		try {
			DT::DataTable dt;
			dt.setPasswordDigest(passwordDigest);
			fb.assign(dt);
			fb.open(profileDir + "cfg.dtb", DT::fileRead);

			fb.close();
		} catch (DT::DTException e) {
			fb.close();
			switch (e.errorCode) {
				case DT::errNotAuthenticated:
				{
					if (passwordDigest.empty() == false) {// has³o by³o ustawiane - trzeba wrzuciæ monit o z³ym haœle...
						IMessage(IMI_ERROR , 0 , 0 , (int)loadString(IDS_ERR_BADPASSWORD).c_str());
					}
					int r = 0;
					if ((r = DialogBox(Stamina::getHInstance(), MAKEINTRESOURCE(IDD_PROFILE), 0, ProfileDialogProc)) != IDCANCEL) return r;
					break;
				}
				case DT::errFileNotFound:
					break;
				default:
					UIMessage(IMI_WARNING , (int)stringf(loadString(IDS_INF_BADFILE).c_str(),fb.getFileName().c_str()).c_str(),0);
					exit(1);
			};
		}


		return 1;
	}

	// Oczyszcza tablice po zaladowaniu z niepotrzebnych smieci...
	void prepareProfile() {
		// CNT
		Tables::cnt->lockData(Tables::allRows);
		for (unsigned int i = 1; i < Tables::cnt->getRowCount() ; i++) {
			int s = Tables::cnt->getInt(i, CNT_STATUS);
			// ST_OFFLINE
			s &= ~(ST_MASK);
			if (s & ST_NOTINLIST) {
				Tables::cnt->removeRow(i--);
				continue;
			}
			Tables::cnt->setInt(i, CNT_STATUS, s | ST_OFFLINE);
		}
		Tables::cnt->unlockData(Tables::allRows);

		// MSG
		Tables::oTableImpl msg (Tables::tableMessages);
		msg->lockData(Tables::allRows);
		for (unsigned int i = 0; i < msg->getRowCount() ; i++) {
			if (msg->getInt(i , MSG_FLAG) & MF_NOSAVE) {
				msg->removeRow(i--);
				continue;
			}
			// Usuwamy zbêdne flagi
			msg->setInt(i, MSG_FLAG, 0, MF_PROCESSING|MF_OPENED);
		}
		msg->unlockData(Tables::allRows);
	}


	int loadProfile(void) {
		Tables::cfg->load();
		Tables::cnt->load();
		Tables::tables[Tables::tableMessages]->load();

		if (Tables::cnt->getRowCount() == 0) {
			Tables::cnt->addRow();
		}
		if (Tables::cfg->getRowCount() == 0) {
			Tables::cfg->addRow();
		}

		prepareProfile();
		IMessage(IM_CFG_LOAD , -1, IMT_CONFIG|IMT_CONTACT , 0 ,0,0);
		return 0;
	}

};
