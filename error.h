#pragma once

namespace Konnekt {

	int except_filter(EXCEPTION_POINTERS xp , const char * threadOwner);
	void exception_information(const char * e);

};