#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include "konnekt_sdk.h"

namespace Konnekt {
#pragma pack(push , 1)
	class cCtrl_ : public cCtrlEx{
		friend class cCtrl1;
	public:

		//   virtual ~cCtrl_(){};
		int __stdcall getLevel() {return 0;}

		unsigned int __stdcall ID(){return _plugin.getId();}  ///< Identyfikator wtyczki
		HINSTANCE __stdcall hInst(){return Stamina::getHInstance();}   ///  Uchwyt procesu (HINSTANCE)
		HINSTANCE __stdcall hDll(){return _plugin.getDllInstance();}   ///  Uchwyt biblioteki.
		int __stdcall getError(); ///< Zwraca kod ostatniego bledu
		void __stdcall setError(int err_code);
		bool __stdcall isRunning() {return ::isRunning;} ///< Ustawia kod b³êdu.

		cCtrl_(Plugin& plugin):_plugin(plugin) {
			warn_cnt = 0; err_cnt = 0; 
			debugLevel = DBG_ALL;
#ifdef __DEBUG
			debugLevel |= DBG_DEBUG;

#endif
		}

		int getWarnCnt() {return warn_cnt;}
		int getErrCnt() {return err_cnt;}
		void _warn(char * ch);
		void _err(char * ch);
		void _fatal(char * ch);

		string last_warn , last_err;      // Ostatni blad i ostrzezenie
	private:
		unsigned int warn_cnt , err_cnt;  // Liczniki ostrzezen i bledow
		Plugin& _plugin;
		unsigned int debugLevel;
	};


	// cCtrl ze zdefiniowanymi funkcjami dla zwyklych dll'ek
	class cCtrl1 : public cCtrl_ {
	public:

		struct BeginThreadParam {
			unsigned int plugID;
			unsigned ( __stdcall *start_address)( void * );
			void * param;
			std::string name;
		};

		cCtrl1(Plugin& plugin):cCtrl_(plugin) {
		}

		// Inne
		int __stdcall IMessage(sIMessage_base * msg);
		int __stdcall IMessageDirect(unsigned int plug , sIMessage_base * msg);
		int __stdcall DTgetPos(tTable db , unsigned int row);
		int __stdcall DTgetID(tTable db , unsigned int row);
		int __stdcall DTgetType(tTable db , unsigned int id);
		int __stdcall DTgetCount(tTable db);
		int __stdcall DTgetNameID(tTable db , const char * name);
		const char * __stdcall DTgetName(tTable db , unsigned int id);
		unsigned int __stdcall Is_TUS(unsigned int thID);
		int __stdcall RecallTS(HANDLE th = 0 , bool wait = 1);
		int __stdcall RecallIMTS(HANDLE th , bool wait , sIMessage_base * msg , int plugID);
		void __stdcall WMProcess();
		void * __stdcall GetTempBuffer(unsigned int size);
		bool __stdcall DTget(tTable db , unsigned int row , unsigned int col , struct Tables::OldValue * value);
		bool __stdcall DTset(tTable db , unsigned int row , unsigned int col , struct Tables::OldValue * value);
		unsigned short __stdcall DTlock(tTable db , unsigned int row , int reserved);
		unsigned short __stdcall DTunlock(tTable db , unsigned int row , int reserved);
		int __stdcall getLevel() {return 1;}
		void * __stdcall malloc(size_t size) {return ::malloc(size);} 
		char * __stdcall strdup(const char * str) {return ::strdup(str);}
		void __stdcall free(void * buff) {::free(buff);}
		int __stdcall DTgetOld(tTable db , unsigned int row , unsigned int col);
		int __stdcall DTsetOld(tTable db , unsigned int row , unsigned int col , int val , int mask=0);
		int __stdcall Sleep(unsigned int time);
		bool __stdcall QuickShutdown() {return !canWait;}

		static unsigned __stdcall BeginThreadRecall( void * tparam);
		static unsigned __stdcall BeginThreadRecallSafe( BeginThreadParam* btp);
		HANDLE __stdcall BeginThreadOld(void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr);

		unsigned int __stdcall DebugLevel(enDebugLevel level = DBG_ALL);
		unsigned int __stdcall SetDebugLevel(enDebugLevel levelMask, enDebugLevel level);			

		 Unique::tId __stdcall getId(Unique::tDomainId domainId, const char * name);
		 const char * __stdcall getName(Unique::tDomainId domainId, Unique::tId id);
		 bool __stdcall idInRange(Unique::tDomainId domainId, Unique::tRangeId rangeId, Unique::tId id);
		 Unique::tRangeId __stdcall idInRange(Unique::tDomainId domainId, Unique::tId id, Unique::iRange::enType check = Unique::iRange::typeBoth);

		Tables::oTable __stdcall getTable(Tables::tTableId tableId);
		Konnekt::oPlugin __stdcall getPlugin(Konnekt::tPluginId pluginId);

		HANDLE __stdcall BeginThread(const char* name, void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr);

		void __stdcall onThreadStart(const char* name=0);
		void __stdcall onThreadEnd();


	};

	// cCtrl z funkcjami troche wyzszego poziomu
	class cCtrl2 : public cCtrl1 {
	public:

		cCtrl2(Plugin& plugin):cCtrl1(plugin) {
		}

		int __stdcall getLevel() {return 2;}
	};

	// cCtrl - odpowiednik cCtrlEx
	class cCtrl3 : public cCtrl2 {
	public:

		cCtrl3(Plugin& plugin):cCtrl3(plugin) {
		}

		int __stdcall getLevel() {return 3;}

		void __stdcall PlugOut(unsigned int id , const char * reason , bool restart = 1);

	};


#pragma pack(pop)
};
