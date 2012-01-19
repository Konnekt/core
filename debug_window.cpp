#include "stdafx.h"
#include "main.h"
#include "debug.h"
#include "resources.h"
#include "plugins.h"
#include "tables.h"
#include "imessage.h"
#include "connections.h"
#include "threads.h"
#include "argv.h"

namespace Konnekt { namespace Debug {

#ifdef __DEBUG
	bool IMfinished = true;
	bool debug=true;

	int x;
	int y;
	int w;
	int h;
	bool show;
	bool log;
	bool scroll;
	bool logAll;
	bool test = false;
	bool showLog = false;
	HWND hwnd = 0 , hwndStatus , hwndLog , hwndBar, hwndCommand;
	HIMAGELIST iml;
	cCriticalSection windowCSection;

	HINSTANCE instRE;

	void testStart();
	void testEnd();
	LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
	unsigned int __stdcall debugThreadProc(void * param);

	// ----------------------------------------------------------

	void startup(HINSTANCE hInst) {
		if (!superUser) return;
		instRE = LoadLibrary("Riched20.dll");

		_beginthreadex(0, 0, debugThreadProc, 0, 0, 0);

		while (!Debug::hwnd) {
			Sleep(0);
		}

		if (getArgV(ARGV_STARTDEV)) {
			Debug::show = true;
			Debug::log = true;
		}

		return;
	}

	void finish() {
		DestroyWindow(Debug::hwnd);
	}


	//char sDebugInfo [MAX_LOADSTRING];
	//char sDebugNew [MAX_LOADSTRING];

	//char sVersionInfo [MAX_LOADSTRING];

	void ShowDebugWnds() {
		if (!superUser) return;
		if (Debug::show) ShowWindow(Debug::hwnd , SW_SHOW);
	}

	void onSizeDebug(int w , int h) {
		SendMessage(Debug::hwndStatus , WM_SIZE , 0,MAKELPARAM(w,h));
		HDWP hDwp;
		RECT rc , rc2;
		GetClientRect(Debug::hwnd , &rc);
		GetClientRect(Debug::hwndBar , &rc2);
		int barHeight = rc2.bottom - rc.top;
		rc.top += rc2.bottom + 2;
		rc.left+=2;
		rc.right-=2;
		GetClientRect(Debug::hwndStatus , &rc2);
		rc.bottom -= rc2.bottom + 2;

		w = rc.right - rc.left;
		h = rc.bottom - rc.top;

		const int commandHeight = 20;
		const int okWidth = 50;
		h -= commandHeight;
		HWND okItem = GetDlgItem(hwnd, IDOK);

		hDwp=BeginDeferWindowPos(2);
		hDwp=DeferWindowPos(hDwp , Debug::hwndBar ,0,
			2,2, w ,barHeight,SWP_NOZORDER);
		hDwp=DeferWindowPos(hDwp , Debug::hwndLog ,0,
			rc.left,rc.top, w ,h,SWP_NOZORDER);
		hDwp=DeferWindowPos(hDwp , Debug::hwndCommand ,0,
			rc.left + okWidth,h+rc.top, w - okWidth ,150,SWP_NOZORDER);
		hDwp=DeferWindowPos(hDwp , okItem ,0,
			rc.left,h+rc.top, okWidth ,commandHeight,SWP_NOZORDER);
		EndDeferWindowPos(hDwp);
	}


	void createWindows() {
#define WS_EX_COMPOSITED        0x02000000L
		if (!Debug::w) {
			Debug::x = Debug::y = 50;
			Debug::w = 400;
			Debug::h = 250;
			Debug::scroll=true;
		}
		Debug::hwnd = CreateDialog(hInst , MAKEINTRESOURCE(IDD_DEBUG) , 0 , (DLGPROC)WndProc);
		Debug::hwndLog = GetDlgItem(Debug::hwnd , IDC_MSG);
		Debug::hwndCommand = GetDlgItem(Debug::hwnd , IDC_COMMAND);
		SetWindowPos(Debug::hwnd , 0 , Debug::x , Debug::y , Debug::w , Debug::h , SWP_NOACTIVATE | SWP_NOZORDER);
		SendMessage(Debug::hwnd , WM_SETICON , ICON_SMALL , (LPARAM)LoadIconEx(hInst , MAKEINTRESOURCE(IDI_DEBUG_ICON) , 16));
		SendMessage(Debug::hwnd , WM_SETICON , ICON_BIG , (LPARAM)LoadIconEx(hInst , MAKEINTRESOURCE(IDI_DEBUG_ICON) , 32));

		Debug::hwndStatus = CreateStatusWindow(SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE,
			("Konnekt "+suiteVersionInfo).c_str(),Debug::hwnd,IDC_STATUSBAR);
		int sbw [3]={200 , -1};
		SendMessage(Debug::hwndStatus , SB_SETPARTS , 2 , (LPARAM)sbw);

		Debug::iml = ImageList_Create(16 , 16 , ILC_COLOR32|ILC_MASK	 , 5 , 5);
		//   icon = LoadIconEx(hInst , MAKEINTRESOURCE(IDI_HISTB_REFRESH) , 16);
		//   ImageList_AddIcon(Debug::iml , icon);
		//   DestroyIcon(icon);


		Debug::hwndBar = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,
			WS_CHILD | WS_CLIPCHILDREN |WS_CLIPSIBLINGS | WS_VISIBLE
			|TBSTYLE_TRANSPARENT
			|CCS_NODIVIDER
			| TBSTYLE_FLAT
			| TBSTYLE_LIST
			//        | CCS_NOPARENTALIGN
			//        | CCS_NORESIZE
			| TBSTYLE_TOOLTIPS
			//| CCS_NOPARENTALIGN
			| CCS_NORESIZE

			, 0, 0, 200, 30, Debug::hwnd,
			(HMENU)IDC_TOOLBAR, hInst, 0);
		// Get the height of the toolbar.
		SendMessage(Debug::hwndBar , TB_SETEXTENDEDSTYLE , 0 ,
			TBSTYLE_EX_MIXEDBUTTONS
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS
			);
		SendMessage(Debug::hwndBar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
		SendMessage(Debug::hwndBar, TB_SETIMAGELIST, 0, (LPARAM)Debug::iml);
		// Set values unique to the band with the toolbar.
		TBBUTTON bb;
		bb.dwData=0;
#define INSERT(cmd , str , bmp , state , style)\
	bb.iString=(int)(str);\
	bb.idCommand=(cmd);\
	bb.iBitmap=(bmp);\
	bb.fsState=(state)| TBSTATE_ENABLED;\
	bb.fsStyle=(style)| BTNS_AUTOSIZE | (bmp==I_IMAGENONE?BTNS_SHOWTEXT:0);\
	SendMessage(Debug::hwndBar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb)
		INSERT(IDB_SHUT , "Zakoñcz" , I_IMAGENONE , 0 , BTNS_CHECK);
		INSERT(0,"",0,0,BTNS_SEP);
		INSERT(IDB_LOG , "Loguj" , I_IMAGENONE , Debug::log?TBSTATE_CHECKED:0 , BTNS_CHECK);
		if (Debug::debugAll) {
			INSERT(IDB_LOGALL , "Wszystko" , I_IMAGENONE , Debug::logAll?TBSTATE_CHECKED:0 , BTNS_CHECK);
		}
		INSERT(IDB_LOGCLEAR , "Wyczyœæ" , I_IMAGENONE , 0 , BTNS_BUTTON);
		INSERT(IDB_MARK , "Zaznacz" , I_IMAGENONE , 0 , BTNS_BUTTON);
		INSERT(0,"",0,0,BTNS_SEP);
		INSERT(IDB_SDK , "SDK" , I_IMAGENONE , 0 , BTNS_BUTTON);
		INSERT(IDB_INFO , "Info" , I_IMAGENONE , 0 , BTNS_BUTTON);
		INSERT(IDB_QUEUE , "Kolejka" , I_IMAGENONE , 0 , BTNS_BUTTON);
		INSERT(IDB_SCROLL , "Przewijaj" , I_IMAGENONE , Debug::scroll?TBSTATE_CHECKED:0 , BTNS_CHECK);
		INSERT(0,"",0,0,BTNS_SEP);
		INSERT(IDB_TEST , "Test" , I_IMAGENONE , 0 , BTNS_CHECK);
		INSERT(IDB_UI , "UI" , I_IMAGENONE , 0 , BTNS_BUTTON);
		//     INSERT(IDB_MSG , "Loguj" , 1 , Debug::log?TBSTATE_CHECKED:0 , BTNS_CHECK);
#undef INSERT
		debugLog();
		onSizeDebug(0,0);
	}

	unsigned int __stdcall debugThreadProc(void * param) {
		// tworzy okno i przyleg³oœci...
		Debug::threadId = GetCurrentThreadId();
		Debug::createWindows();
		MSG msg;
		int bRet;
		while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
		{ 
			if (bRet == -1)
			{
				return 0;// handle the error and possibly exit
			}
			else
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			}
		}
		return 0;
	}


	void runDebugCommand() {
		tStringVector list;
		CStdString command;
		GetWindowText(Debug::hwndCommand, command.GetBuffer(1024), 1024);
		command.ReleaseBuffer();
		SplitCommand(command, ' ', list);
		const char ** argv = new const char * [list.size()];
		for (unsigned int i = 0; i < list.size(); i++) {
			argv[i] = list[i].c_str();
		}
		sIMessage_debugCommand pa(list.size(), argv, sIMessage_debugCommand::asynchronous);
		IMessage(&pa);
		delete [] argv;
	}


	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		//if (hWnd == hwndDebugGSend) MessageBox(0,"ee","",0);
		static bool quitOnce = false;
		string str1,str2;
		switch (message) {
		case WM_CLOSE:
			ShowWindow(hWnd , SW_HIDE);
			break;
		case WM_DESTROY:
			break;
		case WM_SIZING:
			clipSize(wParam , (RECT *)lParam , 250 , 100);
			return 1;
		case WM_SIZE:
			onSizeDebug(LOWORD(lParam) , HIWORD(lParam));
			return 1;
		case WM_COMMAND:
			if (HIWORD(wParam)==BN_CLICKED)	{
				switch (LOWORD(wParam)) {
				case IDB_SHUT:
					//SendMessage(hWnd , WM_ENDSESSION , 0 , 0);break;
					if (quitOnce)
						exit(0);
					else {
						SendMessage(Debug::hwndBar , TB_CHECKBUTTON , IDB_SHUT , 1);
						debugLogMsg("Jeœli naciœniesz [zamknij] jeszcze raz, program zostanie natychmiast przerwany!");
						PostQuitMessage(0);
					}
					quitOnce = true;
					break;
				case IDB_SDK:
					ShellExecute(0 , "open" , resStr(IDS_DEBUG_URL_SDK) , "" , "" , SW_SHOW);
					break;
				case IDB_INFO:
					debugLogInfo();
					break;
				case IDB_QUEUE:
					debugLogQueue();
					break;
				case IDB_LOG:
					Debug::log = !Debug::log;
					debugLogMsg(string("Logowanie ")+(Debug::log?"w³¹czone":"wy³¹czone"));
					SendMessage(Debug::hwndBar , TB_CHECKBUTTON , IDB_LOG , Debug::log);                                
					break;
				case IDB_LOGALL:
					Debug::logAll = !Debug::logAll;
					debugLogMsg(string("Pokazuje ")+(Debug::logAll?"wszystko":"tylko IMC_LOG"));
					SendMessage(Debug::hwndBar , TB_CHECKBUTTON , IDB_LOGALL , Debug::logAll);                                
					break;
				case IDB_SCROLL:
					Debug::scroll = !Debug::scroll;
					SendMessage(Debug::hwndBar , TB_CHECKBUTTON , IDB_LOG , Debug::log);
					break;
				case IDB_TEST:
					Debug::test = !Debug::test;
					SendMessage(Debug::hwndBar , TB_CHECKBUTTON , IDB_TEST , Debug::test);
					if (Debug::test) Debug::testStart();
					else Debug::testEnd();
					break;
				case IDB_LOGCLEAR:
					SetWindowText(Debug::hwndLog , "");
					break;
				case IDB_MARK:
					if (Debug::logFile) {
						static int mark = 0;
						fprintf(Debug::logFile , "\n\n~~~~~~~~~~~~~~~~~~~~ %d ~~~~~~~~~~~~~~~~~~~~~\n\n",++mark);
						fflush(Debug::logFile);
						debugLogMsg(stringf("Zaznaczenie wstawione do "+logFileName+" pod numerem %d",mark));
					}
					break;
				case IDB_UI: IMessageDirect(Plug[0] , IMI_DEBUG);    
					break;
				case IDOK:
					runDebugCommand();
					break;

				}
			}
			break;

			//		default:
			//			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}


	// ---------------------------




#define RE_()   HWND hwnd = Debug::hwndLog;CHARFORMAT2 cf;PARAFORMAT pf;pf.cbSize = sizeof(pf);cf.cbSize = sizeof(cf);
#define RE_PREPARE() {int ndx = GetWindowTextLength (hwnd); SendMessage(hwnd , EM_SETSEL ,  ndx, ndx);}
#define RE_ALIGNMENT(al) pf.dwMask = PFM_ALIGNMENT;pf.wAlignment = al;SendMessage(hwnd , EM_SETPARAFORMAT , 0 , (LPARAM)&pf)
#define RE_FACE(fac) cf.dwMask = CFM_FACE;strcpy(cf.szFaceName , fac);SendMessage(hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf)
#define RE_SIZE(siz) cf.dwMask = CFM_SIZE;cf.yHeight = (siz);SendMessage(hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf)
#define RE_BOLD(bo) cf.dwMask = CFM_BOLD;cf.dwEffects = bo?CFM_BOLD:0;SendMessage(hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf)
#define RE_COLOR(co) cf.dwMask = CFM_COLOR;cf.crTextColor = co;SendMessage(hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf)
#define RE_BGCOLOR(co) cf.dwMask = CFM_BACKCOLOR;cf.crBackColor = co;SendMessage(hwnd , EM_SETCHARFORMAT , SCF_SELECTION , (LPARAM)&cf)
#define RE_ADD(txt) SendMessage(hwnd , EM_REPLACESEL , 0 , (LPARAM)(string(txt).c_str()))
#define RE_ADDLINE(txt) RE_ADD(string(txt) + "\r\n")
#define RE_SCROLLDOWN() {SendMessage(hwnd , WM_VSCROLL , SB_BOTTOM , 0);\
	if (SendMessage(hwnd , EM_GETFIRSTVISIBLELINE , 0 , 0) == SendMessage(hwnd , EM_GETLINECOUNT , 0 , 0)-1) SendMessage(hwnd , EM_SCROLL , SB_PAGEUP , 0);}
	void debugLogInfo(){
		RE_();
		RE_PREPARE();
		RE_COLOR(RGB(0,0,0x80));
		RE_BOLD(1);
		RE_ADD("\r\n\r\n------------ INFO ------------\r\n");
		RE_COLOR(RGB(0xFF,0,0));
		RE_ADD("WTYCZKI:");
		RE_COLOR(0);
		for (int i=0; i<Plug.size(); i++) {
			RE_BOLD(1);
			RE_ADD("\r\n -> " + Plug[i].file);  
			RE_BOLD(0);
			RE_ADD(stringf("\r\n    ID = %d hModule = 0x%x net = %d type = %x prrt = %d " , Plug[i].ID , Plug[i].hModule , Plug[i].net , Plug[i].type ,  Plug[i].priority));
			RE_ADD(stringf("\r\n    name = \"%s\" sig=\"%s\"  v %s" , Plug[i].name.c_str() , Plug[i].sig.c_str() , Plug[i].version.c_str()));
		}

		RE_BOLD(1);
		RE_COLOR(RGB(0xFF,0,0));
		RE_ADD("\r\nCONN:");
		RE_COLOR(0);
		RE_BOLD(0);
		for (Connections::tList::iterator item = Connections::getList().begin(); item != Connections::getList().end(); item++) {
			RE_ADD(stringf("\r\n -> %s %s %d retries" , Plug.Name(item->first) , item->second.connect?"Awaiting connection":"Idle" , item->second.retry));
		}

		RE_ADD(".\r\n");
	}

	void debugLogQueue(){
		RE_();
		RE_PREPARE();
		Msg.lock(DT_LOCKALL);
		for (unsigned int i=0; i<Msg.getrowcount(); i++) {
			RE_BOLD(1);
			RE_COLOR(RGB(0x0,80,0));
			RE_ADD(stringf("\r\nMSG %x %s from '%s' to '%s' [%s]\r\n",Msg.getint(i,MSG_ID) 
				, IMessage(IM_PLUG_NETNAME,Msg.getint(i,MSG_NET),IMT_PROTOCOL)
				, Msg.getch(i,MSG_FROMUID)
				, Msg.getch(i,MSG_TOUID)
				, string(Msg.getch(i,MSG_BODY)).substr(0,30).c_str()));
			RE_BOLD(0);
			RE_COLOR(RGB(0,0,0));
			RE_ADD("EXT []" + string(Msg.getch(i , MSG_EXT)) + "\r\n");
			int flag = Msg.getint(i,MSG_FLAG);
			RE_ADD(stringf("Type=%d  flag=%x " , Msg.getint(i,MSG_TYPE)
				, flag));
			RE_COLOR(RGB(80,0,0));
			RE_ADD(stringf("%s %s %s\r\n" 
				, flag&MF_SEND?"MF_SEND":""
				, flag&MF_PROCESSING?"MF_PROCESSING":""
				, flag&MF_OPENED?"MF_OPENED":""
				));
			RE_COLOR(RGB(0,0,0));
		}
		Msg.unlock(DT_LOCKALL);
		RE_ADD(".\r\n");
	}


	void debugLogPut(string msg) {
		RE_();RE_PREPARE();
		RE_ADD(msg);
	}
	void debugLogMsg(string msg) {
		RE_();RE_PREPARE();
		RE_COLOR(RGB(255,0,0));
		RE_BOLD(1);
		RE_ADD("\r\n\t"+msg+"\r\n\r\n");
		if (Debug::scroll) RE_SCROLLDOWN();
	}
	void debugStatus(int part , string msg) {
		SendMessage(Debug::hwndStatus , SB_SETTEXT , part|SBT_NOBORDERS , (LPARAM)msg.c_str());
	}
	void debugLogValue(string name , string value){

	}

	void debugLog() {
		RE_();
		RE_PREPARE();
		RE_BOLD(1);
		RE_ADD("Konnekt@Dev ");
		RE_BOLD(0);
		RE_ADDLINE(suiteVersionInfo);
		RE_COLOR(RGB(0,0,128));
		string info = _sprintf("ComCtl6_present = %d (%s message loop)\r\n"
#ifdef __DEBUG
			"Wersja DEBUG - logowanie w³¹czone!\r\n"
#endif
#ifdef __WITHDEBUGALL
			"Wersja VERBOSE - mo¿liwoœæ logowania wszystkich wiadomoœci!\r\n"
#endif
			"", isComCtl6 , isComCtl6?"szybszy":"wolniejszy");
		if (Debug::debugAll) {
			info += "logowanie wszystkich wiadomoœci w³¹czone\r\n";
		}
		if (Konnekt::noCatch) {
			info += "logowanie wszystkich wiadomoœci w³¹czone\r\n";
		}
		RE_ADDLINE(info);
		/*		RE_COLOR(RGB(255,0,0));
		RE_BOLD(1);
		RE_ADDLINE("Logowanie do tego okna mo¿e zwiesiæ program w sytuacji gdy dwa w¹tki jednoczeœnie bêd¹ logowa³y! Pracujê nad naprawieniem tej sytuacji...\r\n");
		RE_BOLD(0);*/
		RE_COLOR(RGB(0,0,0));
	}
#define COLOR_SENDER RGB(30,30,30)
#define COLOR_RCVR   RGB(60,60,60)
#define COLOR_LOG    RGB(0,50,0)
#define COLOR_IM     RGB(80,80,80)
#define COLOR_ID     RGB(00,00,80)
#define COLOR_P      RGB(20,20,20)
#define COLOR_RES    RGB(00,80,00)
#define COLOR_ERR    RGB(80,00,00)
#define COLOR_THREAD RGB(100,00,80)
#define COLOR_NR     RGB(200,200,200)

	void debugLogStart(sIMDebug & IMD , sIMessage_base * msg , string ind) {
		if (!superUser || !Debug::log || !Debug::showLog) return;
		if (msg->id != IMC_LOG && !Debug::logAll) return; 
		sIMessage_2params * msg2p = static_cast<sIMessage_2params *>(msg);
		cLocker lock(windowCSection);
		RE_();
		RE_PREPARE();
		RE_BOLD(0);
		RE_COLOR(0);
		if (!IMfinished) RE_ADD("\r\n");
		RE_BGCOLOR(Plug[msg->sender].debugColor);
		RE_ADD("  ");
		RE_BGCOLOR(TLSU().color);
		if (!ind.empty())
			RE_ADD(ind);
		if (msg->id == IMC_LOG) {
			RE_COLOR(COLOR_LOG);
			RE_BOLD(1);
			RE_ADD("## ");
			RE_ADD(IMD.sender);

			bool bold = false;
			switch (msg2p->p2) {
				case DBG_ERROR:
					bold = true;
					RE_COLOR(0x0000FF);
					break;
				case DBG_WARN:
					bold = true;
					RE_COLOR(RGB(0xFF, 0x99, 00));
					break;
				case DBG_TEST_TITLE:
					bold = true;
					RE_COLOR(0x800000);
					break;
				case DBG_TEST_PASSED:
					bold = true;
					RE_COLOR(0x00A000);
					break;
				case DBG_TEST_FAILED:
					bold = true;
					RE_COLOR(0x0000A0);
					break;
			}
			if (!bold) RE_BOLD(0);
			RE_ADD(" \t "+IMD.p1);
			if (bold) RE_BOLD(1);
		} else {
			
			RE_COLOR(COLOR_NR);RE_ADD(IMD.nr+" ");
			RE_COLOR(COLOR_SENDER); RE_ADD(IMD.sender +" -");
			RE_COLOR(COLOR_RCVR); RE_ADD("> "+IMD.receiver);
			RE_COLOR(COLOR_IM);RE_ADD("\t | ");
			RE_COLOR(COLOR_ID);RE_BOLD(1);
			RE_ADD(IMD.id);
			RE_BOLD(0);
			RE_COLOR(COLOR_IM);RE_ADD("\t | ");
			RE_COLOR(COLOR_P);
			RE_ADD(IMD.p1 + " , " + IMD.p2+" ");
		}
		//       fprintf(Debug::logFile , "%s[%s] %s -> %s\t | %s\t | %s %s "
		//             , ind.c_str() , IMD.nr.c_str() , IMD.sender.c_str() , IMD.receiver.c_str()
		//             , IMD.id.c_str() , IMD.p1.c_str() , IMD.p2.c_str()
		//             );
		RE_BGCOLOR(0xffffff);
		if (Debug::scroll) RE_SCROLLDOWN();
	}

	void debugLogEnd(sIMDebug & IMD , sIMessage_base * msg , bool multiline , string ind) {
		if (!superUser || !Debug::log || !Debug::showLog) return;
		if (msg->id != IMC_LOG && !Debug::logAll) return;

		cLocker lock(windowCSection);

		RE_();
		RE_PREPARE();
		if (multiline) {
			RE_ADD(ind);
		} else RE_ADD("\t");
		RE_BGCOLOR(TLSU().color);
		if (msg->id != IMC_LOG) {
			RE_ADD("= ");
			RE_COLOR(COLOR_RES);
			RE_ADD(IMD.result);
			if (TLSU().error.code) {
				RE_COLOR(COLOR_ERR);RE_BOLD(1);
				RE_ADD("\t e"+IMD.error);RE_BOLD(0);
			}
			RE_COLOR(COLOR_THREAD);
			RE_ADD("\t e"+IMD.thread);
		}
		RE_ADD("\r\n");
		RE_BGCOLOR(0xffffff);
		if (Debug::scroll) RE_SCROLLDOWN();
	}

#undef  RE_PREPARE
#undef  RE_ALIGNMENT
#undef  RE_BOLD
#undef  RE_COLOR
#undef  RE_ADD
#undef  RE_ADDLINE

#define TIMEOUT_TEST 2000
	// ------------------------------
	HANDLE dbgEvent=0;
	VOID CALLBACK dbgTestAPC(ULONG_PTR param) {
		if (dbgEvent) SetEvent(dbgEvent);
	}

	void dbgTestThread(void * p) {
		dbgEvent = CreateEvent(0,0,0,0);
		while (Debug::test) {
			debugStatus(1,"APCqueued...");
			int time = GetTickCount();
			QueueUserAPC(dbgTestAPC,hMainThread,0);
			if (WaitForSingleObject(dbgEvent , TIMEOUT_TEST)==WAIT_TIMEOUT) {
				Beep(200,20);
				debugLogMsg(stringf("G³ówny w¹tek jest nieaktywny ponad %.1f s!",(float)(TIMEOUT_TEST/1000)));
				debugStatus(1,"G³ówny w¹tek nie odpowiada!");
			} else {debugStatus(1,stringf("APCtime: %.3f s" , (float)(GetTickCount()-time)/1000,"yo?"));}

			Sleep(2000);
		}
		dbgEvent=0;
		CloseHandle(dbgEvent);
	}

	void testStart() {
		_beginthread(dbgTestThread,0,0);
	}
	void testEnd() {

	}



#endif

};};