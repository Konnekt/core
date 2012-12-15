#include "stdafx.h"
#include "beta.h"
#include "resources.h"
#include "tables.h"
#include "main.h"


// -----------------------------------------------

namespace Konnekt { namespace Beta {

	int CALLBACK BetaDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		static bool PassChanged = false;
		static bool LoginChanged = false;
		switch (message)
		{
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hwnd , IDE_LOGIN));
			PassChanged=false;
			//if (!anonymous) SetDlgItemText(hwnd , IDE_LOGIN , Beta.getch(0,BETA_LOGIN));
			SendDlgItemMessage(hwnd , IDWWW , BM_SETIMAGE , IMAGE_BITMAP , (LPARAM)LoadImage(hInst , "BETA" , IMAGE_BITMAP , 0 , 0 , 0));
			break;
		case WM_NCDESTROY:
			DeleteObject((HBITMAP)SendDlgItemMessage(hwnd , IDWWW , BM_GETIMAGE , 0 , 0));
			hwndBeta=0;
			break;
		case WM_CLOSE:
			wParam = MAKELPARAM(IDCANCEL , BN_CLICKED);
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam))
			{
			case IDWWW:
				ShellExecute(0 , "open" , URL_BETA , "" , "" , SW_SHOW);
				break;
			case IDHELP:
				ShellExecute(0 , "open" , URL_HELP , "" , "" , SW_SHOW);
				break;
			case IDWWWREGISTER:
				ShellExecute(0 , "open" , URL_REGISTER , "" , "" , SW_SHOW);
				break;
			case IDC_STATICREPORT:
				Beta::makeReport("", true, true, false);
				break;
			case IDYES:
				showReport();
				break;
			case IDOK: 
				{
					// zapisuje
					CStdString newLogin;
					GetDlgItemText(hwnd , IDE_LOGIN , newLogin.GetBuffer(50) , 50);
					newLogin.ReleaseBuffer();
					if (LoginChanged && newLogin != betaLogin) {
						if (anonymous && !newLogin.empty()) {
							Beta.setch(0 , BETA_ANONYMOUS_LOGIN , betaLogin.c_str());
						}
						if (!anonymous && newLogin.empty()) {
							// jeœli da siê, wracamy do starego loginu-anonima
							betaLogin = Beta.getch(0 , BETA_ANONYMOUS_LOGIN);
						} else {
							betaLogin = newLogin;
						}
						Beta.setch(0 , BETA_LOGIN , betaLogin.c_str());
						anonymous = newLogin.empty();
						if (anonymous == false) {
							Beta.set64(0 , BETA_LOGIN_CHANGE, _time64(0));
						}
						Beta.setint(0 , BETA_ANONYMOUS , anonymous);
						loggedIn=false;
					}
					if (PassChanged && (anonymous || !newLogin.empty())) {
						CStdString newPass;
						GetDlgItemText(hwnd , IDE_PASS , newPass.GetBuffer(255) , 255);
						newPass.ReleaseBuffer();
						Beta.setch(0 , BETA_PASSMD5 , MD5Hex(newPass , TLS().buff));
						betaPass = Beta.getch(0 , BETA_PASSMD5);
					}
/*					Beta.setint(0, BETA_AFIREWALL , IsDlgButtonChecked(hwnd , IDCB_FIREWALL)==BST_CHECKED);
					Beta.setint(0, BETA_AMODEM , IsDlgButtonChecked(hwnd , IDCB_MODEM)==BST_CHECKED);
					Beta.setint(0, BETA_AWBLINDS , IsDlgButtonChecked(hwnd , IDCB_WINDOWSBLINDS)==BST_CHECKED);*/
					{
						CdtFileBin FBin;
						Tables::savePrepare(FBin);
						FBin.assign(&Beta);
						FBin.save(FILE_BETA);
					}
					Beta::send(true);
				}
				// Fall through.
			case IDCANCEL:
				//                    GetDlgItemText(hwnd , IDE_LOGIN , s_buff , MAX_STRBUFF);
				/*                    if (!*Beta.getch(0 , BETA_LOGIN))
				if (MessageBox(0 , "MUSISZ podaæ login i has³o do swojego konta w systemie Konnekt-beta.\n","" , MB_OKCANCEL|MB_ICONERROR)==IDOK)
				return 0;
				else {if (running) PostQuitMessage(0); else exit(0);}
				*/
				EndDialog(hwnd, wParam);
				return TRUE;
			}
			return 0;
		case EN_CHANGE:
			switch (LOWORD(wParam))
			{
			case IDE_PASS:
				PassChanged=true;
				return TRUE;
			case IDE_LOGIN:
				LoginChanged=true;
				return TRUE;
			}
			break;
			}
			break;


		}
		return 0;
	}


	int CALLBACK ReportDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		switch (message)
		{
		case WM_INITDIALOG:
			if (anonymous) {EndDialog(hwnd , IDCANCEL);}
			SetFocus(GetDlgItem(hwnd , IDE_TITLE));
			SendDlgItemMessage(hwnd , IDE_TYPE , CB_ADDSTRING , 0 , (LPARAM)"Propozycja");
			SendDlgItemMessage(hwnd , IDE_TYPE , CB_ADDSTRING , 0 , (LPARAM)"B³¹d/Problem");
			SendDlgItemMessage(hwnd , IDE_TYPE , CB_SETCURSEL , 0 , 0);

			SendDlgItemMessage(hwnd , IDE_RELATIVE , CB_ADDSTRING , 0 , (LPARAM)"Interfejs");
			SendDlgItemMessage(hwnd , IDE_RELATIVE , CB_ADDSTRING , 0 , (LPARAM)"Rdzeñ");
			SendDlgItemMessage(hwnd , IDE_RELATIVE , CB_ADDSTRING , 0 , (LPARAM)"gg.dll");
			SendDlgItemMessage(hwnd , IDE_RELATIVE , CB_ADDSTRING , 0 , (LPARAM)"sound.dll");
			SendDlgItemMessage(hwnd , IDE_RELATIVE , CB_ADDSTRING , 0 , (LPARAM)"update.dll");

			break;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_NCDESTROY:
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK: {
				int type = SendDlgItemMessage(hwnd , IDE_TYPE , CB_GETCURSEL , 0 , 0)==1;
				GetDlgItemText(hwnd , IDE_RELATIVE , TLS().buff , MAX_STRING);
				string title=TLS().buff;
				title+=":  ";
				GetDlgItemText(hwnd , IDE_TITLE , TLS().buff , MAX_STRING);
				if (!*TLS().buff) {MessageBox(hwnd , "Musisz podaæ tytu³ raportu!" , "" , MB_OK|MB_ICONERROR);return 0;}
				title+=TLS().buff;
				GetDlgItemText(hwnd , IDE_MSG , TLS().buff , MAX_STRING);
				if (!*TLS().buff) {MessageBox(hwnd , "Musisz wpisaæ treœæ raportu!" , "" , MB_OK|MB_ICONERROR);return 0;}
				Beta::report(type , title.c_str() , TLS().buff, 0, info_log(), true);
					   }
					   // Fall through.
			case IDCANCEL:
				EndDialog(hwnd, wParam);
				return TRUE;
			}

		}
		return 0;
	}


	int CALLBACK ErrorDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		ErrorReport * ER;
		switch (message)
		{
		case WM_INITDIALOG: 
			SetProp(hwnd , "ER" , (HANDLE)lParam);
			ER = (ErrorReport*)lParam;
			SetFocus(GetDlgItem(hwnd , IDE_MSG));
			SetDlgItemText(hwnd , IDC_MSG , ER->msg);
			break;

		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
		case IDOK: {
			ER = (ErrorReport*)GetProp(hwnd , "ER");
			GetDlgItemText(hwnd , IDE_MSG , ER->info.GetBuffer(1024) , 1024);
			ER->info.ReleaseBuffer();
				   }
				   // Fall through.
		case IDCANCEL:
			EndDialog(hwnd, wParam);
			return TRUE;
			}

		}
		return 0;
	}


	LRESULT DummyProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
		return DefWindowProc(hwnd , message , wParam , lParam);
	}

};};