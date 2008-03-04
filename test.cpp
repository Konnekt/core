#include "stdafx.h"
#include "main.h"
#include "test.h"
#include "plugins.h"
#include <Stamina\ObjectImpl.h>
#include <Stamina\Array.h>

#include <Konnekt/test_cppunit.h>

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h> 

namespace Konnekt {

	bool getCoreTests(IM::GetTests* gt) {
		cppunit_getTests(gt);
		return true;

	}

	int runCoreTests(IM::RunTests* rt) {

		cppunit_runTests(rt);

		return 0;

	}

};

bool runTest(Plugin* plugin, const StringRef& test, int& sumup) {
	IMDEBUG(DBG_TEST_TITLE, "Test %s :: %s", plugin->getSig().c_str(), test.a_str());

	IM::RunTests rt (new Stamina::Array<IM::TestInfo>());

	rt.tests->append( IM::TestInfo(test) );

	plugin->IMessageDirect(Ctrl, &rt);

	int result = 0;

	for (int i = 0; i < rt.tests->size(); ++i) {
		if (rt.tests->at(i).getFlag(IM::TestInfo::flagFailed)) {
			result ++;
		}
	}

	bool ok = (Ctrl->getError() != Konnekt::errorNoResult ? 1 : 0);
	if (ok) {
		if (result) {
			IMDEBUG(DBG_TEST_FAILED, "Test na %s zakoñczy³ siê NIEPOWODZENIEM! Wyst¹pi³o %d b³êdów!", plugin->getName().a_str(), result);
		} else {
			IMDEBUG(DBG_TEST_PASSED, "Test na %s zakoñczy³ siê powodzeniem!", plugin->getName().a_str(), sumup);
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
				String indent;
				for (int i = 0; i < gt.tests->size(); ++i) {
					IM::TestInfo* ti = &gt.tests->at(i);
					if (ti->name.empty() == false) {
						IMDEBUG(DBG_COMMAND, "%s%s : \"%s\"", indent.a_str(), gt.tests->at(i).name.a_str(), gt.tests->at(i).command.a_str());
					}
					if (ti->getFlag(IM::TestInfo::flagSubOpen)) {
						indent += "  ";
					}
					if (ti->getFlag(IM::TestInfo::flagSubClose)) {
						indent.erase(0, 2);
					}
				}
			} else {
				IMDEBUG(DBG_COMMAND, "Wtyczka %s nie udostêpnia testów!", arg->getArg(1));
			}
		} else {
			IMDEBUG(DBG_COMMAND, "Taka wtyczka nie istnieje!");
		}
	} else {
		int sumup = 0;
		Plugin* plugin = plugins.findSig(arg->getArg(1));
		if (plugin) {
			runTest(plugin, arg->getArg(2), sumup);
		} else {
			IMDEBUG(DBG_COMMAND, "Taka wtyczka nie istnieje!");
		}


	}
#endif

	return 0;
}
