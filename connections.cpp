#include "stdafx.h"
#include "connections.h"
#include "konnekt_sdk.h"
#include "plugins.h"
#include "imessage.h"
#include "tables.h"
#include "argv.h"
#include "pseudo_export.h"

#include <Stamina\Timer.h>

namespace Konnekt { namespace Connections {
	tList list;
	//    VOID CALLBACK timerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
	bool run = false;
	bool firstConnect = true;
	//HANDLE timer;

	Connection & get(int id) {
		return list[id];
	}
	tList & getList() {
		return list;
	}

	void startUp() {
		run = true;
	}

	class ConnectionTimer: public Stamina::Timer {
	public:
		ConnectionTimer() : Timer() {
			run = false;
		}

		void timerProc(__int64 nanotime) {

			bool ok = false;
			bool connected = isConnected();
	
			int period = (Cfg.getint(0,CFG_DIALUP)&&!isConnected())?TIMEOUT_DIAL:TIMEOUT_RETRY;

			//      IMLOG("*ConnTimer %s [%x:%x]" , connected?"Connected":"Disconnected" , dwTimerLowValue , dwTimerHighValue);
			for (tList::iterator list_it = list.begin(); list_it != list.end(); list_it ++) {
				if (list_it->second.connect) {
					if (connected) {
						if (list_it->second.waitTime == 0 || list_it->second.waitTime <= _time64(0)) {
							list_it->second.connect = 0;
							list_it->second.waitTime = 0;
							IMessageDirect(Plug[list_it->first], IM_CONNECT , list_it->second.retry , 0 , 0);
							//          Ctrl.IMessageDirect(IM_CONNECT , Con.list_it->first , 0 , 0 , 0);
							//          IMLOG("  -> %x %d" , Con.list_it->first , Con.list_it->second.retry);
							list_it->second.retry ++;
						} else {
							ok = true;
						}
					}  
				}
			}
			if (ok /*&& Con.timer*/) 
				this->start(period);  //KillTimer(0,Con.timer);

		}


	} timer;

	void connect() {
		firstConnect = false;
		timer.timerProc(0);
	}
	void check() {
		if (!run) return;
		bool ok = false;

		int period = (Cfg.getint(0,CFG_DIALUP)&&!isConnected())?TIMEOUT_DIAL:TIMEOUT_RETRY;

		for (tList::iterator list_it = list.begin(); list_it != list.end(); list_it ++) {
			if (list_it->second.connect) { 
				ok = true;
				if (list_it->second.waitTime == 0 && !firstConnect) {
					list_it->second.waitTime = _time64(0) + (period / 1000);
				}
			}
		}
		if (ok) {
			//        if (!timer)
			//          timer = SetTimer(0 , 0 , (Cfg.getint(0,CFG_DIALUP)&&!CConnected())?TIMEOUT_DIAL:TIMEOUT_RETRY , (TIMERPROC)cConns_timerProc);

			IMLOG("-ConnTimer ON .. in %d ms" , period);

			if (!timer.isRunning()) {
				timer.start(period);
			}

		} else {//if (timer) //KillTimer(0,timer);
			IMLOG("-ConnTimer OFF");
			timer.stop();
		}
	}

	void setConnect(int sender , int connect) {

		if (Connections::run || (Cfg.getint(0, CFG_AUTO_CONNECT) && !getArgV(ARGV_OFFLINE))) {
			IMLOG("* SetConn plug=%x val=%d" , sender , connect);
			list[sender].connect = (connect > 0);
			if (!connect) {
				list[sender].retry = 0;
			}
			if (connect > 2000) {
				list[sender].waitTime = _time64(0) + connect;
			} else {
				list[sender].waitTime = 0;
			}
			check();
		}
	}




	bool isConnected() {
		if (Cfg.getint(0,CFG_DIALUP)) {
			RASCONN TRasCon;
			RASCONNSTATUS Tstatus;

			DWORD lg;
			DWORD lpcon;

			TRasCon.dwSize = 412;
			lg = 256 * TRasCon.dwSize;
			//   IMLOG("*RasEnumConnections");
			if (RasEnumConnections(&TRasCon, &lg, &lpcon) == 0 && lpcon)
			{
				//    IMLOG("  -> Ras [%s] %s | %s" , TRasCon.szEntryName , TRasCon.szDeviceType , TRasCon.szDeviceName);
				Tstatus.dwSize = 160;
				if (!RasGetConnectStatus(TRasCon.hrasconn, &Tstatus)) {
					//      IMLOG("    > [state %d == %d / err %d] %s %s [%d]" , Tstatus.rasconnstate , RASCS_Connected , Tstatus.dwError , Tstatus.szDeviceType , Tstatus.szDeviceName , (Tstatus.rasconnstate == RASCS_Connected));
					if (Tstatus.rasconnstate == RASCS_Connected) return 1;
				}
			}
			return 0;
		} else return 1;
	}


};};