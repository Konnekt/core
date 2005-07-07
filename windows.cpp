#include "stdafx.h"
#include "main.h"
#include "windows.h"
#include "resources.h"
#include "profiles.h"
#include "konnekt_sdk.h"
#include "tables.h"
#include "plugins.h"

#include "include\win_listview.h"
#include <Stamina\ButtonX.h>

namespace Konnekt {

	void InitInstance(HINSTANCE hInst) {
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC  = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);
	}




	// *********************************************
	// Zwraca, czy wybrany profil w ogole istnieje
	bool ProfileDialogFill(HWND hwnd , const char * profile) {
		HWND item = GetDlgItem(hwnd , IDC_COMBO);
		string dir = profilesDir + "*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFile;
		BOOL found;
		found = ((hFile = FindFirstFile(dir.c_str(),&fd))!=INVALID_HANDLE_VALUE);
		int i = 0;
		bool ok = false;
		while (found)
		{
			found = FindNextFile(hFile , &fd);
			if (found && strcmp(fd.cFileName , "..") && fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				SendMessage(item , CB_ADDSTRING , 0 , (LPARAM)fd.cFileName);
				if (!stricmp(profile , fd.cFileName)) {SendMessage(item , CB_SETCURSEL , i , 0);ok=true;}
				i++;
			}
		}
		FindClose(hFile);
		if (!i) {
			SendMessage(hwnd , WM_COMMAND , MAKELPARAM(IDYES , BN_CLICKED) , (LPARAM)GetDlgItem(hwnd , IDYES));
		}
		return ok;
	}

	int CALLBACK ProfileDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		static string start_profile;
		static int badPass = 0;
		switch (message)
		{
		case WM_INITDIALOG:
			SetWindowPos(hwnd , HWND_TOP , 0 , 0 , 0 , 0 , SWP_NOMOVE | SWP_NOSIZE);
			SetFocus(GetDlgItem(hwnd , IDC_PASS1));
			//                SetWindowText(hwnd , dlg->title);
			start_profile = profile;
			//                SetDlgItemText(hwnd , IDC_STATIC1 , _sprintf("Has³o do \"%s\"" , profile.c_str()));
			if (!ProfileDialogFill(hwnd , profile.c_str())) {
				EnableWindow(GetDlgItem(hwnd , IDC_STATIC1) , false);
				//                    EnableWindow(GetDlgItem(hwnd , IDC_PASS1) , false);
			}
			//                SetDlgItemText(hwnd , IDC_EDIT , dlg->ch);
			if (!profile.empty())
				SetFocus(GetDlgItem(hwnd , IDC_PASS1));
			break;
		case WM_CLOSE:
			gracefullExit();
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_NCDESTROY:
			break;
		case WM_COMMAND:
			if (HIWORD(wParam)==BN_CLICKED) {
				switch (LOWORD(wParam))
				{
				case IDYES: {
					sDIALOG_enter sd;
					sd.title = "Zak³adanie nowego profilu";
					sd.info = "Wpisz nazwê nowego profilu";
					sd.value = (char*)profile.c_str();
					memset(md5digest , 0 , 16);
					
					IMessage(IMI_DLGENTER , 0 , 0 , (int)&sd);
					clearChar(sd.value);

					if (!sd.value || !*sd.value) gracefullExit();
					profile = sd.value;
					ShowWindow(hwnd , SW_HIDE);
					EndDialog(hwnd , setProfile(0));
					return 1;
							}
				case IDOK: {
					CStdString login , pass;
					GetDlgItemText(hwnd , IDC_COMBO , login.GetBuffer(255) , 255);
					GetDlgItemText(hwnd , IDC_PASS1 , pass.GetBuffer(255) , 255);
					login.ReleaseBuffer();
					pass.ReleaseBuffer();
					if (start_profile != login) {
						profile = login;
						// zapisujemy na potrzeby kolejnego sprawdzania...
						if (pass.empty())
							memset(md5digest , 0 , 16);
						else
							MD5(pass , md5digest);
						badPass = 0;
						ShowWindow(hwnd , SW_HIDE);
						EndDialog(hwnd , setProfile(0));
						return 1;
					} else {
						if (memcmp(MD5(pass , (unsigned char *) TLS().buff) , md5digest , 16)) {
							IMessage(IMI_ERROR , 0 , 0 , (int)resStr(IDS_ERR_BADPASSWORD));
							//exit(0);
							SetDlgItemText(hwnd , IDC_PASS1 , "");
							if (++badPass >= 3) 
								gracefullExit();
							break;
							/*                         else {
							ShowWindow(hwnd , SW_HIDE);
							EndDialog(hwnd , setProfile(0));
							}*/
						}
					}
					EndDialog(hwnd , IDCANCEL);
					return 1;}
				case IDCANCEL:
					gracefullExit();
					EndDialog(hwnd, IDCANCEL);
					return TRUE;
				}
			}  
			else if (HIWORD(wParam)==CBN_SELCHANGE) {
				SetDlgItemText(hwnd , IDC_PASS1 , "");
				/*               GetDlgItemText(hwnd , IDC_COMBO , TLS().buff , MAX_STRBUFF);
				if (start_profile != TLS().buff) {
				EnableWindow(GetDlgItem(hwnd , IDC_PASS1) , 0);
				} else {
				EnableWindow(GetDlgItem(hwnd , IDC_PASS1) , 1);
				}
				*/
			}

		}
		return 0;
	}



	// ---------------------------------------------------------

	// *******************************************

	bool gotNewPlugins = false;
	enum enSelectAllType {
		selectNew,
		selectAll,
		selectNone,
	} selectAllType = selectAll;

	Stamina::ButtonX* plugBRecommend;
	Stamina::ButtonX* plugBSelect;
	Stamina::ButtonX* plugBDownload;

	Stamina::ButtonX* plugBApply;
	Stamina::ButtonX* plugBCancel;

	HFONT plugSelectFont = Stamina::createFont("Tahoma", 13);
	HFONT plugSelectFontBold = Stamina::createFont("Tahoma", 13, true);
	HFONT plugLegendFont = Stamina::createFont("Tahoma", 12);

	HIMAGELIST plugIml;

	void PlugsDialog(bool firstTimer) {
		DialogBoxParam(hInst , MAKEINTRESOURCE(IDD_PLUGS), 0 , PlugsDialogProc, firstTimer);
	}


	void PlugsDialogRecommend(HWND hwnd) {
		std::list<std::string> list;

		list.push_back("gg.dll");
		list.push_back("expimp.dll");
		list.push_back("kboard.dll");
		list.push_back("kieview.dll");
		list.push_back("kjabber.dll");
		list.push_back("klan.dll");
		list.push_back("konnferencja.dll");
		list.push_back("kstyle.dll");
		list.push_back("ktransfer.dll");
		list.push_back("notify.dll");
		list.push_back("sms.dll");
		list.push_back("sound.dll");
		list.push_back("update.dll");
		list.push_back("faworki.dll");
		list.push_back("kzmieniacz.dll");
		list.push_back("dwutlenek.dll");
		list.push_back("k.lawa.dll");
		list.push_back("ggimage.dll");
		list.push_back("k.away.dll");
		list.push_back("grupy.dll");
		//list.push_back("kpilot2.dll");
		list.push_back("temacik.dll");
		list.push_back("actio.dll");
		list.push_back("ktp.dll");
		list.push_back("checky.dll");

		HWND item = GetDlgItem(hwnd, IDC_LIST);
		int count = ListView_GetItemCount(item);
		for (int i = 0; i<count; i++) {
			int id = ListView_GetItemData(item , i);
			CStdString file = Plg.getch(i,PLG_FILE);
			file.erase( 0, file.find_last_of('\\')+1 );
			file.ToLower();
			ListView_SetCheckState(item , i , (std::find(list.begin(), list.end(), file) != list.end()));
            
		}


	}


	void PlugsDialogSetSelect(HWND hwnd, enSelectAllType select) {
		selectAllType = select;
		HFONT font;
		switch (select) {
			case selectAll:
				plugBSelect->setImage(new Stamina::Icon(Plug[0].hModule, "yes", 16));
				plugBSelect->setText("W³¹cz wszystkie");
				font = plugSelectFont;
				break;
			case selectNone:
				plugBSelect->setImage(new Stamina::Icon(Plug[0].hModule, "no", 16));
				plugBSelect->setText("Wy³¹cz wszystkie");
				font = plugSelectFont;
				break;
			case selectNew:
				plugBSelect->setImage(new Stamina::Icon(hInst, MAKEINTRESOURCE(IDI_PLUG_NEW), 16));
				plugBSelect->setText("W³¹cz nowe");
				font = plugSelectFontBold;
				break;
		}
		Stamina::setWindowFont(plugBSelect->getHWND(), font);

	}

	void PlugsDialogGet(HWND hwnd) {
		HWND item = GetDlgItem(hwnd, IDC_LIST);
		LVITEM li;
		char * ch;
		li.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
		li.iSubItem = 0;
		selectAllType = selectAll;
		gotNewPlugins = false;
		bool gotLoaded = false;
		for (unsigned int i=0;i<Plg.getrowcount();i++) {
			if (Plg.getint(i , PLG_NEW)>=0) {
				ch = strrchr(Plg.getch(i , PLG_FILE) , '\\');
				li.iItem=0x7FFF;

				char str_buff [50]="";
				FILEVERSIONINFO fvi(true);
				FileVersionInfo(Plg.getch(i , PLG_FILE) , str_buff , &fvi);

				//li.pszText = ch?ch+1:Plg.getch(i , PLG_FILE);
				li.pszText = (LPSTR)fvi.InternalName.c_str();
				li.iImage = Plg.getint(i , PLG_LOAD)==-1?3
					:Plg.getint(i , PLG_NEW)==1?1
					:Plg.getint(i , PLG_LOAD)==0?2
					:0;

				if (Plg.getint(i, PLG_NEW) == 1) {
					gotNewPlugins = true;
				}
				if (Plg.getint(i, PLG_LOAD) == 1) {
					selectAllType = selectNone;
					gotLoaded = true;
				}

				li.lParam = Plg.getrowid(i);
				int pos = ListView_InsertItem(item , &li);
				ListView_SetCheckState(item , pos , Plg.getint(i , PLG_LOAD)==1);

				//ListView_SetItemText(item , pos , 1 , (LPSTR)fvi.InternalName.c_str());
				ListView_SetItemText(item , pos , 1 , (LPSTR)fvi.FileDescription.c_str());

				ListView_SetItemText(item , pos , 2 , (LPSTR)(ch?ch+1:Plg.getch(i , PLG_FILE)));
				ListView_SetItemText(item , pos , 3 , str_buff);
				ListView_SetItemText(item , pos , 4 , (LPSTR)fvi.CompanyName.c_str());
				ListView_SetItemText(item , pos , 5 , (LPSTR)fvi.URL.c_str());
			}
		}
		if (gotNewPlugins) {
			selectAllType = selectNew;
		}
		PlugsDialogSetSelect(hwnd, selectAllType);
		if (gotLoaded && gotNewPlugins) {
			ListView_Scroll(item, 0, 0x7FFF);
		}
	}

	void PlugsDialogSet(HWND hwnd) {
		HWND item = GetDlgItem(hwnd, IDC_LIST);
		int count = ListView_GetItemCount(item);
		for (int i = 0; i<count; i++) {
			bool load = ListView_GetCheckState(item , i);
			int pos = Plg.getrowpos(ListView_GetItemData(item , i));

			Plg.setint(pos,PLG_LOAD , load?1:Plg.getint(pos,PLG_LOAD)==-1?-1:0);
			if (pos==i) continue;
			CdtRow * tmp = Plg.rows[i];
			Plg.rows[i]=Plg.rows[pos];
			Plg.rows[pos]=tmp;
		}
		Plg.setindexes();
		Tables::savePlg();

		

	}

	int CALLBACK PlugsDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		//   static char str_buff2 [MAX_STRBUFF];
		static HIMAGELIST iml;
		static long param;
		static bool changed;
		HWND item;
		HICON icon;
		int c;
		switch (message)
		{
		case WM_INITDIALOG:
			changed=false;
			param = lParam;
			item = GetDlgItem(hwnd, IDC_LIST);
			iml = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK ,4,4);
			plugIml = iml;
			icon = LoadIconEx(hInst , MAKEINTRESOURCE(IDI_PLUG_OK) , 16);
			ImageList_AddIcon(iml , icon);
			DestroyIcon(icon);
			icon = LoadIconEx(hInst , MAKEINTRESOURCE(IDI_PLUG_NEW) , 16);
			ImageList_AddIcon(iml , icon);
			DestroyIcon(icon);
			icon = LoadIconEx(hInst , MAKEINTRESOURCE(IDI_PLUG_BAN) , 16);
			ImageList_AddIcon(iml , icon);
			DestroyIcon(icon);
			icon = LoadIconEx(hInst , MAKEINTRESOURCE(IDI_PLUG_ERROR) , 16);
			ImageList_AddIcon(iml , icon);
			DestroyIcon(icon);
			ListView_SetImageList(item , iml , LVSIL_SMALL);

			ListView_AddColumn(item , "Nazwa" , 100);
			ListView_AddColumn(item , "Opis" , 200);
			ListView_AddColumn(item , "Plik" , 70);
			ListView_AddColumn(item , "Wersja" , 50);
			ListView_AddColumn(item , "Autor" , 80);
			ListView_AddColumn(item , "URL" , 100);
			SendMessage(item , LVM_SETEXTENDEDLISTVIEWSTYLE , 0 ,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

			plugBRecommend = new Stamina::ButtonX(GetDlgItem(hwnd, IDC_RECOMMEND));
			plugBRecommend->setImage(new Stamina::Icon(Ctrl->hInst(), "MAINICON", 16));
			Stamina::setWindowFont(plugBRecommend->getHWND(), plugSelectFontBold);
			plugBSelect = new Stamina::ButtonX(GetDlgItem(hwnd, IDC_SELECT));
			plugBDownload = new Stamina::ButtonX(GetDlgItem(hwnd, IDC_DOWNLOAD));
			plugBDownload->setImage(new Stamina::Icon(Plug[0].hModule, "url", 16));
			Stamina::setWindowFont(plugBDownload->getHWND(), plugSelectFont);

			plugBApply = new Stamina::ButtonX(GetDlgItem(hwnd, IDOK));
			plugBApply->setImage(new Stamina::Icon(Plug[0].hModule, "apply", 16));
			plugBCancel = new Stamina::ButtonX(GetDlgItem(hwnd, IDCANCEL));
			plugBCancel->setImage(new Stamina::Icon(Plug[0].hModule, "cancel", 16));

			Stamina::setWindowFont(GetDlgItem(hwnd, IDC_LEGEND), plugLegendFont);
			Stamina::setWindowFont(GetDlgItem(hwnd, IDC_STATIC2), plugLegendFont);
			Stamina::setWindowFont(GetDlgItem(hwnd, IDC_STATIC4), plugLegendFont);
			Stamina::setWindowFont(GetDlgItem(hwnd, IDC_STATIC6), plugLegendFont);


			//                SendDlgItemMessage(hwnd , IDC_UP , WM_SETFONT , (WPARAM)fontDing , 1);
			//                SendDlgItemMessage(hwnd , IDC_DOWN , WM_SETFONT , (WPARAM)fontDing , 1);
			PlugsDialogGet(hwnd);
			SetForegroundWindow(hwnd);
			SetActiveWindow(hwnd);
			if (lParam) {
				PlugsDialogRecommend(hwnd);
			}
			break;
		case WM_CLOSE:
			EndDialog(hwnd , IDCANCEL);
			break;
		case WM_NCDESTROY:
			ImageList_Destroy(iml);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_SELECT: {
				char type [2];
				HWND item = GetDlgItem(hwnd, IDC_LIST);
				GetWindowText((HWND)lParam , type , 2);
				int count = ListView_GetItemCount(item);
				for (int i = 0; i<count; i++) {
					if (selectAllType == selectNew) {
						int pos = ListView_GetItemData(item , i);
						if (Plg.getint(pos,PLG_NEW)>0) {
							ListView_SetCheckState(item , i , true);
						}
					} else {
						ListView_SetCheckState(item , i , selectAllType == selectAll);
					}
				}
				PlugsDialogSetSelect( hwnd, selectAllType == selectAll ? selectNone : selectAll );
				break;}
			case IDC_RECOMMEND: {
				PlugsDialogRecommend(hwnd);
				break;}
			case IDC_DOWNLOAD:
				ShellExecute(0, "OPEN", "doc\\download.html", "", "", SW_SHOW);
				break;
			case IDOK: {

				HWND item = GetDlgItem(hwnd, IDC_LIST);
				int count = ListView_GetItemCount(item);
				bool load = false;
				for (int i = 0; i<count; i++) {
					if (ListView_GetCheckState(item , i)) 
						load = true;
				}
				if (!load && count > 0) {
					int r = IMessage(IMI_CONFIRM , 0,0,(int)"Nie wybra³eœ ¿adnych wtyczek!\r\nKonnekt bez wtyczek praktycznie nic nie potrafi, jesteœ tego pewien?", MB_ICONWARNING | MB_YESNO);
					if (r == IDNO)
						return 0;
				}

				PlugsDialogSet(hwnd);
				if (running && changed && IMessage(IMI_CONFIRM , 0,0,(int)"Konnekt musi zostaæ uruchomiony ponownie aby zmiany odnios³y skutek.\n\nZrestartowaæ?")) {
					deInit(false);
					restart();}
				}
			case IDCANCEL:
				EndDialog(hwnd , wParam);
				break;
			}
			break;
		case WM_NOTIFY:
			NMHDR * nm;nm=(NMHDR*)lParam;
			NMLVKEYDOWN * nkd;
			switch (nm->code) {
		case LVN_KEYDOWN: {
			nkd = (NMLVKEYDOWN *)lParam;
			if (!(GetKeyState(VK_CONTROL)&0x80)&&!(GetKeyState(VK_MENU)&0x80)) return 0;
			if (nkd->wVKey==VK_UP || nkd->wVKey==VK_DOWN) {
				bool up=nkd->wVKey==VK_UP?1:0;
				item=GetDlgItem(hwnd,IDC_LIST);
				c=ListView_GetItemCount(item);
				if (c<2) return 0;
				for (int i=(up?1:c-2); (up?i<c:i>=0); (up?i++:i--)) {
					if (ListView_GetItemState(item,i,LVIS_SELECTED))
						ListView_MoveItem(item , i , i+(up?-1:1));
				}
				changed=true;
			}
			break;}
		case NM_CLICK: changed=true;break;  
			}
			break;

		}
		return 0;
	}
};