#pragma once

#include "konnekt_sdk.h"
#include <Konnekt/plugin_test.h>


namespace Konnekt {

	bool getCoreTests(IM::GetTests* gt);
	int runCoreTests(IM::RunTests* rt);


	int command_test(sIMessage_debugCommand * arg);
};