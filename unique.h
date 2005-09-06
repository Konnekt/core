#pragma once

#include <Stamina/UniqueImpl.h>

#include "main.h"
#include "konnekt_sdk.h"
#include "konnekt/core_unique.h"

namespace Konnekt { namespace Unique {
	using namespace Stamina::Unique;

	void initializeUnique();
	
	void debugCommand(sIMessage_debugCommand * arg);
};};