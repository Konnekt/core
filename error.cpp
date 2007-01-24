#include "stdafx.h"
#include "error.h"
#include "beta.h"
#include "debug.h"

#include <Stamina\Exception.h>

using Stamina::RegEx;

using namespace Stamina;

namespace Konnekt {

	int except_filter(EXCEPTION_POINTERS xp , const char * threadOwner) {
		volatile static LONG recurse = 0; // Tutaj mo¿na trafiæ tylko raz!
		if (InterlockedExchange(&recurse, 1)) {
			if (Debug::logFile) {
				fprintf(Debug::logFile , "\n\nANOTHER Critical Structured Exception caught!!! Aborting...");
				fflush(Debug::logFile);
			}
			exit(1);
		}

		SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);


		CStdString log = Beta::info_log();
 		if (Debug::logFile) {
			fprintf(Debug::logFile , "\nLocking...");
			fflush(Debug::logFile);
		}

		Beta::lockdown();

		if (Debug::logFile) {
			fprintf(Debug::logFile , "\nLock applied");
			fflush(Debug::logFile);
		}


		if (Debug::logFile) {
			fprintf(Debug::logFile , "\n\nCritical Structured Exception caught. Preping more info...");
			fflush(Debug::logFile);
		}
		Debug::stackTrace=dcallstack(xp.ContextRecord , false);


		EXCEPTION_RECORD xr = *xp.ExceptionRecord;
		string exName;
		string exInfo;
		switch (xr.ExceptionCode) {
		case CRITICAL_SECTION_TIMEOUT_EXCEPTION: exName="Critical Section Timeout";break;
		case EXCEPTION_ACCESS_VIOLATION: exName="Access Violation";
			if (xr.NumberParameters > 1) {
				exInfo = stringf("can't %s %0x", xr.ExceptionInformation[0] ? "write" : "read", xr.ExceptionInformation[1]);
            }
			
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: exName="ARRAY_BOUNDS_EXCEEDED";break;
		case EXCEPTION_BREAKPOINT: exName="BREAKPOINT";break;
		case EXCEPTION_DATATYPE_MISALIGNMENT: exName="DATATYPE_MISALIGNMENT";break;
		case EXCEPTION_FLT_DENORMAL_OPERAND: exName="FLT_DENORMAL_OPERAND";break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: exName="FLT_DIVIDE_BY_ZERO";break;
		case EXCEPTION_FLT_INEXACT_RESULT: exName="FLT_INEXACT_RESULT";break;
		case EXCEPTION_FLT_INVALID_OPERATION: exName="FLT_INVALID_OPERATION";break;
		case EXCEPTION_FLT_OVERFLOW: exName="FLT_OVERFLOW";break;
		case EXCEPTION_FLT_STACK_CHECK: exName="FLT_STACK_CHECK";break;
		case EXCEPTION_FLT_UNDERFLOW: exName="FLT_UNDERFLOW";break;
		case EXCEPTION_ILLEGAL_INSTRUCTION: exName="ILLEGAL_INSTRUCTION";break;
		case EXCEPTION_IN_PAGE_ERROR: exName="IN_PAGE_ERROR";break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO: exName="INT_DIVIDE_BY_ZERO";break;
		case EXCEPTION_INT_OVERFLOW: exName="INT_OVERFLOW";break;
		case EXCEPTION_INVALID_DISPOSITION: exName="INVALID_DISPOSITION";break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: exName="NONCONTINUABLE_EXCEPTION";break;
		case EXCEPTION_PRIV_INSTRUCTION: exName="PRIV_INSTRUCTION";break;
		case EXCEPTION_SINGLE_STEP: exName="SINGLE_STEP";break;
		case EXCEPTION_STACK_OVERFLOW: exName="STACK_OVERFLOW";break;
		case EXCEPTION_PURE_VIRTUAL_CALL: exName="PURE_VIRTUAL_CALL";
			if (xr.NumberParameters > 0 && xr.ExceptionInformation[0]) {
				exInfo = stringf("from %.100s handler", xr.ExceptionInformation[0]);
            }
			break;
		default: exName = stringf("Unknown Exception [%x]",xr.ExceptionCode);break;
		}

		CStdString msg = "Critical structured Exception \"" + exName + "\" occured at adress 0x";
		msg +=inttostr((int)xr.ExceptionAddress,16,8);
		if (MainThreadID==GetCurrentThreadId())
			msg +=" in the main Thread.";
		else 
			msg +=" in the \"" + string(threadOwner) + "\" thread.";

		CStdString toDigest;
		toDigest.Format("Structured Exception %s ", exName.c_str(), threadOwner?threadOwner:"");

		if (exInfo.empty()) {
			for (unsigned int i=0; i<xr.NumberParameters; i++) {
				exInfo+="[0x"+inttostr(xr.ExceptionInformation[i],16,2)+"] ";
			}
		}
		if (!exInfo.empty()) {
			msg +="\nInformation: ";
			msg += exInfo;
		}
		msg +="\n";
		CStdString addInfo;
		Beta::imdigest(addInfo);
		Beta::stackdigest(addInfo);

		toDigest += RegEx::doReplace("/0x\\d+/", "", addInfo).c_str();

		msg += addInfo;
		Beta::errorreport(REPTYPE_ERROR , (exName/*+" at 0x"+inttostr((int)xr.ExceptionAddress,16,8)*/) , msg, Beta::makeDigest(toDigest).c_str(), log);
		//  xp.ContextRecord->
		//  printf("\n x [%x]" , xr.ExceptionCode);

		//  MessageBox(NULL ,_sprintf(resStr(IDS_ERR_EXCEPTION),exName.c_str()),"",MB_ICONERROR|MB_OK|MB_TASKMODAL );

		return 1;
	}

	void exception_information(const char * e) {
		CStdString msg = stringf("Unhandled Exception \"%s\" occured.\n\n" , e);
		CStdString toDigest = msg;
		CStdString addInfo;
		Beta::imdigest(addInfo);
		Debug::stackTrace=dcallstack(0 , false);
		Beta::stackdigest(addInfo);
		toDigest += RegEx::doReplace("/0x\\d+/", "", addInfo).c_str();
		msg += addInfo;
		Beta::errorreport(REPTYPE_EXCEPTION , e , msg, Beta::makeDigest(toDigest).c_str(), Beta::info_log());
		//  printf("\n X %s @ %d" , __throwFileName , __throwLineNumber);
	}

};