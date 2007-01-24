#include "stdafx.h"
#include <cppunit/extensions/HelperMacros.h>


class TestPlugin : public CPPUNIT_NS::TestFixture
{
  
	CPPUNIT_TEST_SUITE( TestPlugin );
  
//	CPPUNIT_TEST( testSetCount );

	CPPUNIT_TEST_SUITE_END();

protected:

public:
	void setUp() {
	}
	void tearDown() {
	}

protected:


};

CPPUNIT_TEST_SUITE_REGISTRATION( TestPlugin );

 