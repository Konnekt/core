#include "stdafx.h"
#include "main.h"
#include "test.h"
#include "plugins.h"
#include <Stamina\ObjectImpl.h>
#include <Stamina\Array.h>

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h> 

namespace Konnekt {

	bool getCoreTests(IM::GetTests* gt) {
	
		return true;

	}

	int runCoreTests(sIMessage_plugArgs * arg) {

		return 0;

	}

};

bool runTest(Plugin* plugin, sIMessage_debugCommand * arg, int& sumup) {
	IMDEBUG(DBG_TEST_TITLE, "Test %s :: %s", plugin->getSig().c_str(), arg->getArg(2));
	sIMessage_plugArgs testArg(arg->argc, arg->argv);
	testArg.id = IM::runTests;
	int result = plugin->IMessageDirect(Ctrl, &testArg);
	bool ok = (Ctrl->getError() != Konnekt::errorNoResult ? 1 : 0);
	if (ok) {
		if (result) {
			IMDEBUG(DBG_TEST_FAILED, "Test na %s zakoñczy³ siê NIEPOWODZENIEM! Wyst¹pi³o %d b³êdów!", plugin->getName().c_str(), result);
		} else {
			IMDEBUG(DBG_TEST_PASSED, "Test na %s zakoñczy³ siê powodzeniem!", plugin->getName().c_str(), sumup);
		}
		sumup += result;
		return true;
	} else {
		IMDEBUG(DBG_TEST_PASSED, "Wtyczka nie obs³uguje tego testu...");
		return false;
	}
}

int Konnekt::command_test(sIMessage_debugCommand * arg) {
#ifdef __TEST

	if (arg->argc < 2) {
		IMDEBUG(DBG_COMMAND, "U¿ycie: test SIG TEST");
		IMDEBUG(DBG_COMMAND, "SIG - sygnatura wtyczki, lub '*' dla wszystkich");
		IMDEBUG(DBG_COMMAND, "TEST - test do wykonania, lub '*' dla ogólnego");
		IMDEBUG(DBG_COMMAND, "¯eby zobaczyæ dostêpne testy wpisz: test SIG");
	} else if (arg->argc < 3) {
		Plugin* plugin = plugins.findSig(arg->getArg(1));
		if (plugin) {
			IM::GetTests gt(new Array<IM::TestInfo>);
			if (plugin->IMessageDirect(Ctrl, &gt)) {
				IMDEBUG(DBG_COMMAND, "Dostêpne testy:");
				for (int i = 0; i < gt.tests->size(); ++i) {
					IMDEBUG(DBG_COMMAND, "%s : \"%s\"", gt.tests->at(i).name.a_str(), gt.tests->at(i).command.a_str());
				}
			} else {
				IMDEBUG(DBG_COMMAND, "Wtyczka %s nie udostêpnia testów!", arg->getArg(1));
			}
		} else {
			IMDEBUG(DBG_COMMAND, "Taka wtyczka nie istnieje!");
		}
	} else {
		int sumup = 0;
		if (arg->argEq(1, "*")) {
			int run = 0;
			IMDEBUG(DBG_COMMAND, "Wykonujê test na wszystkich wtyczkach!");
			for (Plugins::tList::iterator it = plugins.begin(); it != plugins.end(); ++it) {
				run += runTest(it->get(), arg, sumup);
			}

			if (sumup) {
				IMDEBUG(DBG_TEST_FAILED, "Test %s :: %s zakoñczy³ siê NIEPOWODZENIEM! Wyst¹pi³o %d b³êdów!", arg->getArg(1), arg->getArg(2), sumup);
			} else {
				IMDEBUG(DBG_TEST_PASSED, "Test %s :: %s zakoñczy³ siê powodzeniem!", arg->getArg(1), arg->getArg(2), sumup);
			}

		} else {
			Plugin* plugin = plugins.findSig(arg->getArg(1));
			if (plugin) {
				runTest(plugin, arg, sumup);
			} else {
				IMDEBUG(DBG_COMMAND, "Taka wtyczka nie istnieje!");
			}
		}


	}
#endif

	return 0;
}
