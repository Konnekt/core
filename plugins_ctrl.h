#pragma once

#include "main.h"
#include "konnekt_sdk.h"

namespace Konnekt {
#pragma pack(push , 1)
	class Controler_ : public ControlerEx{
		friend class Controler1;
	public:

		//   virtual ~Controler_(){};
		int __stdcall getLevel() {return 0;}

		tPluginId __stdcall ID(){return _plugin.getId();}  ///< Identyfikator wtyczki
		HINSTANCE __stdcall hInst(){return Stamina::getHInstance();}   ///  Uchwyt procesu (HINSTANCE)
		HINSTANCE __stdcall hDll(){return _plugin.getDllModule();}   ///  Uchwyt biblioteki.
		enIMessageError __stdcall getError(); ///< Zwraca kod ostatniego bledu
		void __stdcall setError(enIMessageError err_code);
		bool __stdcall isRunning() {return ::isRunning;} ///< Ustawia kod b³êdu.

		Controler_(Plugin& plugin):_plugin(plugin) {
			warn_cnt = 0; err_cnt = 0; 
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
	};


	// Controler ze zdefiniowanymi funkcjami dla zwyklych dll'ek
	class Controler1 : public Controler_ {
	public:

		struct BeginThreadParam {
			unsigned int plugID;
			unsigned ( __stdcall *start_address)( void * );
			void * param;
			std::string name;
		};

		Controler1(Plugin& plugin):Controler_(plugin) {
		}

		// Inne
		int __stdcall IMessage(sIMessage_base * msg);
		int __stdcall IMessageDirect(tPluginId plug , sIMessage_base * msg);
		int __stdcall DTgetPos(tTable db , unsigned int row);
		int __stdcall DTgetID(tTable db , unsigned int row);
		int __stdcall DTgetType(tTable db , unsigned int id);
		int __stdcall DTgetCount(tTable db);
		int __stdcall DTgetNameID(tTable db , const char * name);
		const char * __stdcall DTgetName(tTable db , unsigned int id);
		unsigned int __stdcall Is_TUS(unsigned int thID);
		int __stdcall RecallTS(HANDLE th = 0 , bool wait = 1);
		int __stdcall RecallIMTS(HANDLE th , bool wait , sIMessage_base * msg , tPluginId plugID);
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

		unsigned int __stdcall getPluginsCount() {
			return plugins.count();
		}

		class Stamina::Logger* __stdcall getLogger();
		void __stdcall logMsg(enDebugLevel level, const char* module, const char* where, const char* msg);


	};

	// Controler z funkcjami troche wyzszego poziomu
	class Controler2 : public Controler1 {
	public:

		Controler2(Plugin& plugin):Controler1(plugin) {
		}

		int __stdcall getLevel() {return 2;}
	};

	// Controler - odpowiednik ControlerEx
	class Controler3 : public Controler2 {
	public:

		Controler3(Plugin& plugin):Controler2(plugin) {
		}

		int __stdcall getLevel() {return 3;}

		void __stdcall PlugOut(unsigned int id , const char * reason , bool restart = 1);

	};


#pragma pack(pop)
};
