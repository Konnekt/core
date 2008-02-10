#include "stdafx.h"
#include <cppunit/extensions/HelperMacros.h>

/*
		if (arg->argEq(1, "crash")) {
			if (arg->argEq(2, "thread")) {
				Ctrl->BeginThread("Crashtest", 0, 0, makeCrash);
			} else if (arg->argEq(2, "threadThrow")) {
				Ctrl->BeginThread("Crashtest", 0, 0, makeCrashThrow);
			} else if (arg->argEq(2, "throw")) {
				makeCrashThrow(0);
			} else if (arg->argEq(2, "threadVirtual")) {
				Ctrl->BeginThread("Crashtest", 0, 0, makePureVirtualCall);
			} else if (arg->argEq(2, "virtual")) {
				makePureVirtualCall(0);
			} else {
				makeCrash(0);
			}
		}
*/

class TestCrash : public CPPUNIT_NS::TestFixture
{
  
	CPPUNIT_TEST_SUITE( TestCrash );
  
	CPPUNIT_TEST( thread );
	CPPUNIT_TEST( threadThrow );
	CPPUNIT_TEST( threadVirtual );
	CPPUNIT_TEST( mainThrow );
	CPPUNIT_TEST( mainVirtual );

	CPPUNIT_TEST_SUITE_END();

protected:

public:
	void setUp() {
	}
	void tearDown() {
	}

protected:

	void thread() {
		CPPUNIT_ASSERT( 1 == 1 );
		CPPUNIT_ASSERT( 1 == 0 );
	}

	void threadThrow() {
	}

	void threadVirtual() {
	}

	void mainThrow() {
	}

	void mainVirtual() {
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( TestCrash );

/*

unsigned int __stdcall makeCrash(void* p) {
	char * crash = 0;
	crash[0] = 0;
	return 0;
}
unsigned int __stdcall makeCrashThrow(void* p) {
	throw "Crash";
	return 0;
}

class PureVirtualTest {
public:
	virtual ~PureVirtualTest() {
		this->test();
	}
	void test() {
		this->pure();
	}
	virtual void pure() = 0;
};
class PureVirtualTest2:public PureVirtualTest {
public:
	void pure() {}
};


unsigned int __stdcall makePureVirtualCall(void*pa) {
	PureVirtualTest* p = new PureVirtualTest2();
	delete p;
	return 0;
}
*/
