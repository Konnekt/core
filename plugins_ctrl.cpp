#include "stdafx.h"
#include "tables.h"
#include "plugins.h"
#include "plugins_ctrl.h"
#include "main.h"
#include "imessage.h"
#include "threads.h"
#include "konnekt_sdk.h"
#include "resources.h"
#include "debug.h"
#include "error.h"
#include "unique.h"

#include <stamina\Thread.h>
#include <stamina\Exception.h>

using namespace Stamina;

namespace Konnekt {


	//-----------------------------------------------

	void Controler_::setCtrl(int id , void * handle) {_ID = id; _hDll = (HINSTANCE)handle;}
	void Controler_::_warn(char * ch) {
		warn_cnt++;
		if (ch && *ch) last_warn = ch;
	}
	void Controler_::_err(char * ch) {
		err_cnt++;
		if (ch && *ch) last_err = ch;
	}
	void Controler_::_fatal(char * ch) {
		_err(ch);
	}

	enIMessageError __stdcall Controler_::getError() {
		return TLSU().stack.getError();
	}

	void __stdcall Controler_::setError(enIMessageError err_code) {
		TLSU().stack.setError(err_code);
	}



	//-----------------------------------------------

	int __stdcall Controler1::IMessage(sIMessage_base * msg){
		msg->sender = this->ID();
		return dispatchIMessage(msg);
	}

	int __stdcall Controler1::IMessageDirect(unsigned int plugId , sIMessage_base * msg) {
		Plugin* plugin = plugins.get(plugId);
		if (plugin == 0) {
			_err("IMessage sent in space ...");
			TLSU().setError(IMERROR_BADPLUG);
			return 0;
		}
		return plugin->sendIMessage(this, msg);
	}

//#define CTRL_SETDT CdTable * DT = (db==DTCFG)?&Cfg : (db==DTCNT)?&Cnt : (db==DTMSG)?&Msg : 0;\

#define CTRL_SETDT Tables::oTable DT (Tables::getTable((tTableId)db));\
	if (!DT) _fatal("Table access denied , or table doesn\'t exist")

	bool __stdcall  Controler1::DTget(tTable db , unsigned int row , unsigned int col , Tables::OldValue * value) {
		if (!value) return false;
		CTRL_SETDT;

		if (value->type == DT_CT_UNKNOWN) {
			value->type = DT->getColumn(col)->getType();
		}

		switch (value->type) {
			case DT_CT_INT:
				value->vInt = DT->getInt(row, col);
				return true;
			case DT_CT_64:
				value->vInt64 = DT->getInt64(row, col);
				return true;
			case DT_CT_STRING:
				String s = DT->getString(row, col);
				if (!value->vChar || !value->buffSize) {
					value->vChar = (char*)this->GetTempBuffer(s.getLength() + 1);
                    value->buffSize = s.getLength() + 1;
				}
				strncpy(value->vChar, s.a_str(), value->buffSize);
				value->vChar[value->buffSize-1] = 0;
				return true;
		};
		return false;

	}

	bool __stdcall  Controler1::DTset(tTable db , unsigned int row , unsigned int col , Tables::OldValue * value) {
		if (!value) return false;
		CTRL_SETDT;

		switch (value->type) {
			case DT_CT_INT:
				DT->setInt(row, col, value->vInt);
				return true;
			case DT_CT_64:
				DT->setInt64(row, col, value->vInt64);
				return true;
			case DT_CT_STRING:
				DT->setString(row, col, value->vCChar);
				return true;
		}

		return false;

	}

	int __stdcall   Controler1::DTgetOld(tTable db , unsigned int row , unsigned int col) {
		throw ExceptionDeprecated("DTgetOld");
		//return (int)this->DTget(row,col, &DT::OldValue());
	}
	int __stdcall   Controler1::DTsetOld(tTable db , unsigned int row , unsigned int col , int value , int mask) {
		throw ExceptionDeprecated("DTsetOld");
	}


	int __stdcall Controler1::DTgetType (tTable db , unsigned int col) {
		CTRL_SETDT;
		return DT->getColumn(col)->getType();
	}

	int __stdcall Controler1::DTgetNameID(tTable db , const char * name) {
		CTRL_SETDT;
		return DT->getColumn(name)->getId();

	}
	const char * __stdcall Controler1::DTgetName(tTable db , unsigned int col) {
		CTRL_SETDT;
		String& buff = TLSU().buffer().getString(true);
		buff = DT->getColumn(col)->getName();
		return buff.c_str();
	}


	int __stdcall Controler1::DTgetPos(tTable db , unsigned int row){
		CTRL_SETDT;
		return DT->getRowPos(row);
	}

	int __stdcall Controler1::DTgetID(tTable db , unsigned int row){
		CTRL_SETDT;
		return DT->getRowId(row);
	}

	int __stdcall Controler1::DTgetCount(tTable db) {
		CTRL_SETDT;
		return DT->getRowCount();
	}

	unsigned short __stdcall Controler1::DTlock(tTable db , unsigned int row , int reserved) {
		CTRL_SETDT;
		DT->lockData(row);
		return 1;
	}
	unsigned short __stdcall Controler1::DTunlock(tTable db , unsigned int row , int reserved){
		CTRL_SETDT;
		DT->unlockData(row);
		return 1;
	}



	unsigned int __stdcall Controler1::Is_TUS(unsigned int thID) {
		if (!thID) thID = MainThreadID;
		return (hMainThread && (GetCurrentThreadId()!=thID)) ? thID : 0;
	}


	int __stdcall Controler1::RecallTS(HANDLE th  , bool wait) {
		if (TLSU().lastIM.inMessage<=0) return 0;
		return ::RecallIMTS(TLSU().lastIM , th , wait);
	}
	int __stdcall Controler1::RecallIMTS(HANDLE th  , bool wait , sIMessage_base * msg , int plugID){
		sIMTS imts;
		imts.msg = msg;
		imts.msg->sender = this->ID();
		imts.plugID = plugID;
		return ::RecallIMTS(imts , th , wait);
	}

	void __stdcall Controler1::WMProcess () {
		if (GetCurrentThreadId()==MainThreadID)
			mainWindowsLoop();
		else 
			processMessages(0);
		//  else _warn("It's useless to call WMProcess outside the main thread!");
	}


	void * __stdcall Controler1::GetTempBuffer(unsigned int size) {
		return TLSU().buffer(this->ID()).getBuffer(size);
	}

	int __stdcall Controler1::Sleep(unsigned int time) {
		int r=MsgWaitForMultipleObjectsEx(0 , 0 , time , QS_ALLINPUT | QS_ALLPOSTMESSAGE , (canWait ? MWMO_ALERTABLE : 0) | MWMO_INPUTAVAILABLE);
		if (r == WAIT_OBJECT_0 + 0)
			WMProcess();
		return r;
	}

	unsigned __stdcall Controler1::BeginThreadRecallSafe( BeginThreadParam* btp) {
		__try { // C-style exceptions
			return btp->start_address(btp->param);
		} 
		__except(except_filter((*GetExceptionInformation()), btp->name.c_str())) 
		{
			exit(1);
		}
		return 0;
	}


	unsigned __stdcall Controler1::BeginThreadRecall( void * tparam) {
		BeginThreadParam* btp = (BeginThreadParam*)tparam;
		Stamina::Thread::setName(btp->name.c_str(), -1);
		unsigned int r;
		if (noCatch) {
			r = btp->start_address(btp->param);
		} else {
			r = BeginThreadRecallSafe(btp);
		}
		//Ctrl->IMDEBUG(DBG_MISC, "BeginThread(%s) finished", btp->name.c_str());
		delete btp;
        return r;
	}

	HANDLE __stdcall Controler1::BeginThreadOld(void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr) {
		return this->BeginThread(0, security, stack_size, start_address, arglist, initflag, thrdaddr);
	}


	HANDLE __stdcall Controler1::BeginThread(const char* name, void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr) {
#ifndef __NOCATCH
		BeginThreadParam * btp = new BeginThreadParam;
		btp->param = arglist;
		btp->start_address = start_address;
		btp->plugID = this->_ID;
		if (name) {
			K_ASSERT( K_CHECK_PTR( name ) );
		}
		btp->name = string(Plug.FindID(btp->plugID)!=-1? Plug[Plug.FindID(btp->plugID)].name.c_str() : "unknown") + "." + string((name && *name) ? name : "unnamed");
		//Ctrl->IMDEBUG(DBG_FUNC, "BeginThread(%s)", btp->name.c_str());
		HANDLE handle = (HANDLE)_beginthreadex(security , stack_size , BeginThreadRecall , btp , initflag , thrdaddr);
		return handle;
#else
		return (HANDLE)_beginthreadex(security , stack_size , start_address , arglist , initflag , thrdaddr);
#endif
	}

	unsigned int __stdcall Controler1::DebugLevel(enDebugLevel level) {
		return this->_plugin.getLogger()->getLevel(level);
	}
	unsigned int __stdcall Controler1::SetDebugLevel(enDebugLevel levelMask, enDebugLevel level) {
		return this->_plugin.getLogger()->setLevel(level, levelMask);
	}


	Unique::tId Controler1::getId(Unique::tDomainId domainId, const char * name) {
		return Unique::getId(domainId, name);
	}
	const char * Controler1::getName(Unique::tDomainId domainId, Unique::tId id) {
		String& buff = TLSU().buffer().getString(true);
		buff = Unique::getName(domainId, id).a_str();
		return buff.c_str();
	}

	bool Controler1::idInRange(Unique::tDomainId domainId, Unique::tRangeId rangeId, Unique::tId id) {
		Unique::oDomain d = Unique::getDomain(domainId);
		if (!d) return false;
		return d->idInRange(rangeId, id);
	}

	Unique::tRangeId Controler1::idInRange(Unique::tDomainId domainId, Unique::tId id, Unique::Range::enType check){
		Unique::oDomain d = Unique::getDomain(domainId);
		if (!d) return Unique::rangeNotFound;
		return d->inRange(id, check)->getRangeId();
	}


	Tables::oTable __stdcall Controler1::getTable(Tables::tTableId tableId) {
		return Tables::getTable(tableId);
	}
	Konnekt::oPlugin __stdcall Controler1::getPlugin(Konnekt::tPluginId pluginId) {
		return oPlugin();
		/*TODO: getPlugin*/
	}

	void __stdcall Controler1::onThreadStart(const char* name) {
		Stamina::LockerCS lock(threadsCS);
		if (threads.find(GetCurrentThreadId())==threads.end()) {
			ThreadInfo& info = threads[GetCurrentThreadId()];
			if (name) {
				Stamina::Thread::setName(name, -1);
				info.name = name;
			}
		}
	}
	void __stdcall Controler1::onThreadEnd() {
		Stamina::LockerCS lock(threadsCS);
		threads.erase(GetCurrentThreadId());
		TLSU.detach(); 
	}

	class Stamina::Logger* __stdcall Controler1::getLogger() {
		this->_plugin.getLogger().get();
	}
	void __stdcall Controler1::logMsg(enDebugLevel level, const char* module, const char* where, const char* msg) {
		this->_plugin.getLogger()->logMsg(level, module, where, msg);
	}


	//-----------------------------------------------

	void __stdcall Controler3::PlugOut(unsigned int id , const char * reason , bool restart) {
		id = Plug.FindID(id);
		if (id == -1) return;
		Tables::oTableImpl plg(tablePlugins);
		tRowId pl = plg->findRow(0, DT::Find::EqStr(plg->getColumn(PLG::file), Plug[id].file)).getId();
		if (pl == DT::rowNotFound) return;
		CStdString msg = loadString(IDS_ERR_DLL);
		msg.Format(msg , Plug[id].file.c_str() , reason);
		if (MessageBox(NULL , msg , loadString(IDS_APPNAME).c_str() , MB_ICONWARNING|MB_OKCANCEL|MB_TASKMODAL) == IDOK) {
			plg->setInt(pl , PLG::load , -1);
			plg->save();
		}

#ifdef __DEBUG
		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n\nPlug %s made a booboo ...\n  \"%s\"\n\n" , Plug[id].file.c_str() , reason );
		}
#endif
		if (!running)
			exit(0);
		else ::IMessage(IMC_RESTART , 0 , 0);
	}


	Controler_ * createPluginCtrl(Plugin& plugin) {
		return new Controler1(plugin);
	}
	Controler_ * createCorePluginCtrl(Plugin& plugin) {
		return new Controler3(plugin);
	}


};