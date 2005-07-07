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

namespace Konnekt {


	//-----------------------------------------------

	void cCtrl_::setCtrl(int id , void * handle) {_ID = id; _hDll = (HINSTANCE)handle;}
	void cCtrl_::_warn(char * ch) {
		warn_cnt++;
		if (ch && *ch) last_warn = ch;
	}
	void cCtrl_::_err(char * ch) {
		err_cnt++;
		if (ch && *ch) last_err = ch;
	}
	void cCtrl_::_fatal(char * ch) {
		_err(ch);
	}

	int __stdcall cCtrl_::getError() {
		return TLSU().error.code;
	}

	void __stdcall cCtrl_::setError(int err_code) {
		TLSU().setError(err_code);
	}



	//-----------------------------------------------

	int __stdcall cCtrl1::IMessage(sIMessage_base * msg){
		msg->sender = this->ID();
		return IMessageProcess(msg , 0);
	}

	int __stdcall cCtrl1::IMessageDirect(unsigned int plug , sIMessage_base * msg) {
		msg->sender = this->ID();
		int i=Plug.FindID(plug);
		if (i<0) {_err("IMessage sent in space ...");
		TLSU().setError(IMERROR_BADPLUG);return 0;}
		/*
		if (msg->id < IM_SHARE && (msg->sender != ui_sender && plug!=msg->sender)
		&& !(!plug && msg->id<IM_BASE)) 
		{IMBadSender(msg , plug);return 0;}
		*/
		return Konnekt::IMessageDirect(Plug[i],msg);
	}

#define CTRL_SETDT CdTable * DT = (db==DTCFG)?&Cfg : (db==DTCNT)?&Cnt : (db==DTMSG)?&Msg : 0;\
	if (!DT) _fatal("Table access denied , or table doesn\'t exist")

	bool __stdcall  cCtrl1::DTget(tTable db , unsigned int row , unsigned int col , Tables::Value * value) {
		if (!value) return false;
		CTRL_SETDT;
		if (value->type == DT_CT_PCHAR && !value->vCChar && value->buffSize==0)
			value->vCChar = TLS().buff2;
		return DT->getValue(row , col , value->dtRef());

	}

	bool __stdcall  cCtrl1::DTset(tTable db , unsigned int row , unsigned int col , Tables::Value * value) {
		if (!value) return false;
		CTRL_SETDT;
		return DT->setValue(row , col , value->dtRef());

	}

	int __stdcall   cCtrl1::DTgetOld(tTable db , unsigned int row , unsigned int col) {
		CTRL_SETDT;
		return (int)DT->get(row,col);
	}
	int __stdcall   cCtrl1::DTsetOld(tTable db , unsigned int row , unsigned int col , int value , int mask) {
		CTRL_SETDT;
		if (!mask) {
			if (db==DTCNT && col == CNT_STATUS) mask = ST_MASK;
			else mask = -1;
		}  
		if (mask == -1)
			return DT->set(row , col , (TdEntry)value);
		else
			return DT->set(row , col, (TdEntry)(((int)DT->get(row , col) & (~mask)) | (value)));
	}


	int __stdcall cCtrl1::DTgetType (tTable db , unsigned int col) {
		CTRL_SETDT;
		int i=DT->cols.colindex(col);
		if (i<0) return 0;
		return DT->cols.getcolflag(i);
	}

	int __stdcall cCtrl1::DTgetNameID(tTable db , const char * name) {
		CTRL_SETDT;
		return DT->cols.getnameid(name);

	}
	const char * __stdcall cCtrl1::DTgetName(tTable db , unsigned int col) {
		CTRL_SETDT;
		int i=DT->cols.colindex(col);
		if (i<0) return 0;
		return DT->cols.getcolname(i);
	}


	int __stdcall cCtrl1::DTgetPos(tTable db , unsigned int row){
		CTRL_SETDT;
		return DT->getrowpos(row);
	}

	int __stdcall cCtrl1::DTgetID(tTable db , unsigned int row){
		CTRL_SETDT;
		return DT->getrowid(row);
	}

	int __stdcall cCtrl1::DTgetCount(tTable db) {
		CTRL_SETDT;
		return DT->getrowcount();
	}

	unsigned short __stdcall cCtrl1::DTlock(tTable db , unsigned int row , int reserved) {
		CTRL_SETDT;
		return DT->lock(row);
	}
	unsigned short __stdcall cCtrl1::DTunlock(tTable db , unsigned int row , int reserved){
		CTRL_SETDT;
		return DT->unlock(row);
	}



	unsigned int __stdcall cCtrl1::Is_TUS(unsigned int thID) {
		if (!thID) thID = MainThreadID;
		return (hMainThread && (GetCurrentThreadId()!=thID)) ? thID : 0;
	}


	int __stdcall cCtrl1::RecallTS(HANDLE th  , bool wait) {
		if (TLSU().lastIM.inMessage<=0) return 0;
		return ::RecallIMTS(TLSU().lastIM , th , wait);
	}
	int __stdcall cCtrl1::RecallIMTS(HANDLE th  , bool wait , sIMessage_base * msg , int plugID){
		sIMTS imts;
		imts.msg = msg;
		imts.msg->sender = this->ID();
		imts.plugID = plugID;
		return ::RecallIMTS(imts , th , wait);
	}

	void __stdcall cCtrl1::WMProcess () {
		if (GetCurrentThreadId()==MainThreadID)
			WMessages();
		else 
			ProcessMessages(0);
		//  else _warn("It's useless to call WMProcess outside the main thread!");
	}


	void * __stdcall cCtrl1::GetTempBuffer(unsigned int size) {
		return TLSU().Buffers[this->ID()].GetBuffer(size);
	}

	int __stdcall cCtrl1::Sleep(unsigned int time) {
		int r=MsgWaitForMultipleObjectsEx(0 , 0 , time , QS_ALLINPUT | QS_ALLPOSTMESSAGE , (canWait ? MWMO_ALERTABLE : 0) | MWMO_INPUTAVAILABLE);
		if (r == WAIT_OBJECT_0 + 0)
			WMProcess();
		return r;
	}

	unsigned __stdcall cCtrl1::BeginThreadRecallSafe( BeginThreadParam* btp) {
		__try { // C-style exceptions
			return btp->start_address(btp->param);
		} 
		__except(except_filter((*GetExceptionInformation()), btp->name.c_str())) 
		{
			exit(1);
		}
		return 0;
	}


	unsigned __stdcall cCtrl1::BeginThreadRecall( void * tparam) {
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

	HANDLE __stdcall cCtrl1::BeginThreadOld(void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr) {
		return this->BeginThread(0, security, stack_size, start_address, arglist, initflag, thrdaddr);
	}


	HANDLE __stdcall cCtrl1::BeginThread(const char* name, void *security,unsigned stack_size,unsigned ( __stdcall *start_address )( void * ),void *arglist,unsigned initflag,unsigned *thrdaddr) {
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

	unsigned int __stdcall cCtrl1::DebugLevel(enDebugLevel level) {
		return this->debugLevel & level;
	}
	unsigned int __stdcall cCtrl1::SetDebugLevel(enDebugLevel levelMask, enDebugLevel level) {
		return this->debugLevel = (this->debugLevel & ~levelMask) | level;
	}


	Unique::tId cCtrl1::getId(Unique::tDomainId domainId, const char * name) {
		return Unique::getId(domainId, name);
	}
	const char * cCtrl1::getName(Unique::tDomainId domainId, Unique::tId id) {
		return Unique::getName(domainId, id);
	}
	bool cCtrl1::idInRange(Unique::tDomainId domainId, Unique::tRangeId rangeId, Unique::tId id) {
		return Unique::idInRange(domainId, rangeId, id);
	}
	Unique::tRangeId cCtrl1::idInRange(Unique::tDomainId domainId, Unique::tId id, Unique::Range::enType check){
		return Unique::idInRange(domainId, id, check);
	}


	Tables::oTable __stdcall cCtrl1::DT(Tables::tTableId tableId) {
		return Tables::getTable(tableId);
	}
	Konnekt::oPlugin __stdcall cCtrl1::getPlugin(Konnekt::tPluginId pluginId) {
		return oPlugin();
		/*TODO: getPlugin*/
	}

	void __stdcall cCtrl1::onThreadStart(const char* name) {
		Stamina::LockerCS lock(threadsCS);
		if (threads.find(GetCurrentThreadId())==threads.end()) {
			HANDLE copy;
			DuplicateHandle(GetCurrentProcess() , GetCurrentThread() , GetCurrentProcess() , &copy , 0 , 0 , DUPLICATE_SAME_ACCESS);
			threads[GetCurrentThreadId()]=copy;
			if (name) {
				Stamina::Thread::setName(name, -1);
				TLSU().name = name;
			}
		}
	}
	void __stdcall cCtrl1::onThreadEnd() {
		Stamina::LockerCS lock(threadsCS);
		if (threads.find(GetCurrentThreadId())!=threads.end()) {
			CloseHandle(threads[GetCurrentThreadId()]);
			threads.erase(GetCurrentThreadId());
		}
		TLS.detach();
		TLSU.detach(); 
	}


	//-----------------------------------------------

	void __stdcall cCtrl3::PlugOut(unsigned int id , const char * reason , bool restart) {
		id = Plug.FindID(id);
		if (id == -1) return;
		int pl = Plg.findby((TdEntry)Plug[id].file.c_str() , PLG_FILE);
		if (pl < 0) return;
		CStdString msg = resStr(IDS_ERR_DLL);
		msg.Format(msg , Plug[id].file.c_str() , reason);
		if (MessageBox(NULL , msg , resStr(IDS_APPNAME) , MB_ICONWARNING|MB_OKCANCEL|MB_TASKMODAL) == IDOK) {
			Plg.setint(pl , PLG_LOAD , -1);
			Tables::savePlg();
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


	cCtrl_ * createPluginCtrl() {
		return new cCtrl1;
	}
	cCtrl_ * createCorePluginCtrl() {
		cCtrl3 * c = new cCtrl3;
		c->setCtrl(0, 0);
		return c;
	}


};