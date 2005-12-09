#pragma once

#include "stdafx.h"

#include "main.h"
#include "resources.h"
#include "konnekt_sdk.h"
#include "beta.h"
#include "debug.h"
#include "plugins.h"
#include "plugins_ctrl.h"
#include "imessage.h"
#include "profiles.h"
#include "tables.h"
#include "connections.h"
#include "contacts.h"
#include "messages.h"
#include "windows.h"
#include "error.h"
#include "unique.h"
#include "argv.h"
#include "mru.h"

#include <Stamina\Logger.h>
#include <Stamina\Exception.h>
#include <Stamina\Debug.h>
#include "KLogger.h"

#include <Konnekt\Lib.h>

#pragma comment(lib,"Ws2_32.lib")

IMPLEMENT_PURECALL_EXCEPTION("Konnekt.exe");

using namespace Stamina;

namespace Konnekt {
	using namespace Debug;

	CStdString appPath;
	CStdString appFile;
	CStdString sessionName; // ID sesji - MD5 katalogu z profilem
	CStdString tempPath;
	CStdString dataPath;
	HINTERNET hInternet = 0; // uchwyt do globalnej sesji sieciowej...
	bool noCatch = false;
	//	bool suspended = false;

	CStdString versionInfo;
	unsigned int versionNumber;
	CStdString  suiteVersionInfo;
	bool newVersion = true;

	HANDLE hMainThread;
	unsigned long MainThreadID; 
	bool canQuit = true;
	bool running = false;
	bool isRunning = true;
	bool isWaiting = false;
	bool canWait = true; // Czy mo¿na czekaæ na cokolwiek (overlapped IO itd...)
	HANDLE semaphore;



	void mainWindowsLoop(bool loop) {
		MSG msg;
		HWND fhWnd=0;
		static HANDLE nullEvent = CreateEvent(0, 0, 0, 0);
		while (1) {
			if (canWait) 
				/* Zatkany message loop blokuje funkcje APC
				, jednorazowo przegladamy wiec kolejke. nullEvent nigdy nie bedzie zasygnalizowany... */
				WaitForSingleObjectEx(nullEvent, 0, TRUE);
			if (!loop) {
				if (!PeekMessage(&msg , 0 , 0 , 0 , PM_REMOVE)) return;
			} else {
				isWaiting=true;
				int wait_ret=MsgWaitForMultipleObjectsEx(0 , 0 , INFINITE , QS_ALLINPUT | QS_ALLPOSTMESSAGE , (canWait ? MWMO_ALERTABLE : 0) | MWMO_INPUTAVAILABLE);
				isWaiting=false;
				if (wait_ret == MWMO_ALERTABLE || !PeekMessage(&msg, NULL, 0, 0 , PM_REMOVE)) {
					continue;
				}
			}
			if (!msg.hwnd && (msg.message == WM_QUIT)) {
				if (!loop) { 
					IMLOG("- Received WM_QUIT outside of loop");
					deinitialize();
					return;
				}
				IMLOG("- Received WM_QUIT!"); return;
			}
			if (msg.message == WM_TIMECHANGE) {
				Beta::setStats(Beta::lastStat);
			} else if (msg.message == WM_ENDSESSION) {
				IMLOG("- WM_ENDSESSION caught, continue = %d" , !running);
				if (!running)
					continue;
				else
					deinitialize(true);

			}
			fhWnd = GetForegroundWindow();
			if (!fhWnd || !IsDialogMessage(fhWnd,&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	void mainProgramBlock(void) {
		Konnekt::initialize();
		Konnekt::mainWindowsLoop(1);
		Konnekt::deinitialize();
	}

	void mainSafeBlock2(void) {
#if !defined(__NOCATCH) || defined(__PLUGCATCH)
		try {
#endif
			mainProgramBlock();
#ifndef __NOCATCH
		}
		catch (int e) {
			//STACKTRACE();
			exception_information(inttostr(e,16,2).c_str());
			//break;//exit(0);
		}
		catch (std::exception & e) {
			//STACKTRACE();
			exception_information(e.what());
			//break;//exit(0);
		}
		catch (Stamina::Exception& e) {
			exception_information(e.getReason().c_str());
		}
		catch (char * e) {
			//STACKTRACE();
			exception_information(e);
			//break;
		}
		catch (string& e) {
			//STACKTRACE();
			exception_information((char*)e.c_str());
			//break;
		}
#endif
#if !defined(__NOCATCH) || defined(__PLUGCATCH)
#if defined(__NOCATCH)
		}
#endif
		catch (exception_plug e) {
			if (plugins.exists(e.id)) plugins[e.id].madeError(e.msg , e.severity);
		}
		catch (exception_plug2 e) {
			if (plugins.exists(e.id)) plugins[e.id].madeError(e.msg , e.severity);
		}
		/*    #if !defined(__NOCATCH)
		catch (...) {
		exception_information("Unknown");
		}
		#endif */
#endif 
	}

	void mainSafeBlock(void) {
#ifndef __NOCATCH
		__try { // C-style exceptions
#endif
			mainSafeBlock2();
#ifndef __NOCATCH
		} 
		__except(except_filter((*GetExceptionInformation()),"Core")) 
		{
			IMLOG("Aborting...");
			exit(1);
		}
#endif // NOCATCH
	}



//------

	void __cdecl atexit_finish ( void ){
		static bool once = false;
		if (once) return;
		once = true;

#ifdef __DEBUG
		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n\nGracefull Exit\n");

			Stamina::debugDumpObjects(new LoggerFile(Debug::logFile, Stamina::logAll));

			fclose(Debug::logFile);
			unlink(logPath + "konnekt_bck.log");
			rename(logPath + "konnekt.log" , logPath + "konnekt_bck.log");
			rename(logFileName , logPath + "konnekt.log");
		}
#endif
	}

	void gracefullExit() {
		atexit_finish();
		exit(0);
	}


	void __cdecl restart ( void ) {
		string args;
		if (!getArgV("-restart" , false)) {
			args += "-restart";
		}
		for (int i=1; i < __argc; i++) {
			if (!args.empty()) {
				args += " ";
			}
			args += "\"";
			args += __argv[i];
			args += "\"";
		}

#ifdef __DEBUG
		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n\nRestarting: %s %s\n" , appFile.c_str() , args.c_str());
			fclose(Debug::logFile);
			Debug::logFile = 0;
		}
#endif

		atexit_finish();
		ShellExecute(0 , "open" , appFile , args.c_str() , "" , SW_SHOW);
		//   _execl(_argv[0] , _argv[0] , "/restart");
		gracefullExit();
	}

	void __cdecl removeProfileAndRestart ( void ) {
		Stamina::removeDirTree((char*)profileDir.c_str());
		restart();
	}

	bool restoreRunningInstance() {
		HWND wnd = FindWindow("UItop."+sessionName , 0);
		if (!wnd) {
			wnd = FindWindow("UImain" , 0); // niech bêdzie dowolny inny...
			if (wnd)
				wnd = GetParent(wnd);
		}
		if (!wnd) return false;
		/*	COPYDATASTRUCT cds;
		// trzeba przygotowaæ kopiê danych...
		cds.dwData = __argc;
		cds.cbData = (__argc + 1) * sizeof(int);
		// obliczamy rozmiar i wype³niamy offsety
		*/
		char ** data = new char* [__argc];//(int *)malloc(cds.cbData);
		int maxSize = __argc * sizeof(char*);
		for (int i=0; i < __argc; i++) {
			data[i] = (char*)maxSize;
			maxSize+=strlen(__argv[i])+1;
		}
		//	data[__argc] = (char*) maxSize;

		HANDLE fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE , 0 , PAGE_READWRITE , 0 , maxSize , "Local\\KONNEKT.File.RestoreData");
		char * base = (char *)MapViewOfFile(fileMapping , FILE_MAP_WRITE , 0 , 0 , maxSize);
		IMLOG("- restore - sending sz=%d" , maxSize);
		memcpy(base , data , __argc * sizeof(char*));
		for (int i=0; i < __argc; i++) {
			strcpy(base + (int)data[i] , __argv[i]);
			IMLOG("      +data %x=%s" , data[i] , base+(int)data[i]);
		}
		int r = SendMessage(wnd , RegisterWindowMessage("KONNEKT.Msg.RestoreData") , __argc , maxSize);
		//	cds.lpData = data;
		//	int r = SendMessage(wnd , WM_COPYDATA , 0 , (LPARAM)&cds);
		UnmapViewOfFile(base);
		CloseHandle(fileMapping);
		delete [] data;
		return r==TRUE;
	}


	WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{

		Stamina::setHInstance(hInstance);

#ifdef __DEBUGMEM
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | /*_CRTDBG_CHECK_ALWAYS_DF |*/ _CRTDBG_CHECK_EVERY_16_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

		showArgumentsHelp(false);

		initializePaths();

		initializeSuiteVersionInfo();

		loadRegistry();

		initializeProfileDirectory();

		initializeInstance(hInstance);

		Debug::initializeDebug();

	#if defined(__NOCATCH) && !defined(__PLUGCATCH)
		noCatch = true;
		mainProgramBlock();
	#else
		noCatch = getArgV(ARGV_NOCATCH);
		if (noCatch) {
			mainProgramBlock();
		} else {
			mainSafeBlock();
		}
	#endif
		return 0;
	}



};


WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return Konnekt::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
