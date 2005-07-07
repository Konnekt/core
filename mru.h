#pragma once

#include "konnekt_sdk.h"

namespace Konnekt { namespace MRU {
	bool update(sMRU * mru);
	int get(sMRU * mru);
	bool set(sMRU * mru);
};};
