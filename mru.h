#pragma once

#include "konnekt_sdk.h"

namespace Konnekt { namespace MRU {

	extern tTableId tableMRU;

	void init();

	bool update(sMRU * mru);
	int get(sMRU * mru);
	bool set(sMRU * mru);
};};
