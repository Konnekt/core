#include "stdafx.h"
#include "main.h"
#include "test.h"
#include <Stamina\ObjectImpl.h>

class TestClassObj: public Stamina::SharedObject<Stamina::iSharedObject> {
public:
	typedef Stamina::SharedObject<Stamina::iSharedObject> parent_type;
	TestClassObj(const CStdString & id) {
		this->id = id;
		IMDEBUG(DBG_TEST, "%s::ctor", id.c_str());
		counter++;
	}
	~TestClassObj() {
		IMDEBUG(DBG_TEST, "%s::dtor", id.c_str());
		counter--;
	}
	bool __stdcall hold() {
		IMDEBUG(DBG_TEST, "%s::hold", id.c_str());
		return parent_type::hold();
	}
	void __stdcall release() {
		IMDEBUG(DBG_TEST, "%s::release", id.c_str());
		parent_type::release();
	}
	void __stdcall destroy() {
		IMDEBUG(DBG_TEST, "%s::destroy", id.c_str());
		parent_type::destroy();
	}

    CStdString id;	
	static int counter;
};
int TestClassObj::counter = 0;
class TestClassObj2: public TestClassObj {
public:
	TestClassObj2(const CStdString & id):TestClassObj(id) {
	}
	void __stdcall destroy() {
		{
			IMDEBUG(DBG_TEST_TITLE, "%s::destroy_rec.test", id.c_str());
			Stamina::SharedPtr<TestClassObj2> test2(*this);
			testResult(0, test2->getUseCount());	
			Stamina::SharedPtr<TestClassObj2> test3;
			test3 = test2;
			testResult(0, test2->getUseCount());	
		}
		TestClassObj::destroy();
	}
};


/*class TestClass: public KObject<TestClassObj> {
};*/

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

void Konnekt::command_test(sIMessage_debugCommand * arg) {
#ifdef __TEST
	if (arg->argc < 2) {
		IMDEBUG(DBG_COMMAND, "- Dostêpne testy:");
		IMDEBUG(DBG_COMMAND, " object");

	} else {
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
		} else if (arg->argEq(1, "object")) {
			IMDEBUG(DBG_TEST_TITLE, "- Konnekt::KObject {{{");

			typedef Stamina::SharedPtr<TestClassObj> TestClass;
			IMDEBUG(DBG_TEST_TITLE, "Create - Delete");
			{
				TestClass t1 (new TestClassObj("o1"));
			}
			IMDEBUG(DBG_TEST_TITLE, "Create empty - set - Delete");
			{
				TestClass t1;
				t1 = new TestClassObj("o2");
			}
			IMDEBUG(DBG_TEST_TITLE, "Create - copy - Delete");
			{
				TestClass t1 (new TestClassObj("o3"));
				{
					IMDEBUG(DBG_TEST, "copy");
					TestClass t2 (t1);
				}
				testResult(2, t1->getUseCount());
			}
			IMDEBUG(DBG_TEST_TITLE, "Create x2 - Delete");
			{
				TestClass t1 (TestClass(new TestClassObj("o4")));
			}
			IMDEBUG(DBG_TEST_TITLE, "Create1 - Create2 - Delete1 Delete2");
			{
				TestClass * t1 = new TestClass(new TestClassObj("o5"));
				IMDEBUG(DBG_TEST, "Create2");
				TestClass * t2 = new TestClass(*t1);
				IMDEBUG(DBG_TEST, "Delete1");
				delete t1;
				testResult(2, (*t2)->getUseCount());
				IMDEBUG(DBG_TEST, "Delete2");
				delete t2;
			}
			IMDEBUG(DBG_TEST_TITLE, "Create - hold - copy - release");
			{
				TestClassObj * o = new TestClassObj("o6");
				o->hold();
				{
					IMDEBUG(DBG_TEST, "set");
                    TestClass t1(o);
					testResult(3, o->getUseCount());
				}
				testResult(2, o->getUseCount());
                o->release();
			}
			IMDEBUG(DBG_TEST_TITLE, "Createx2 - swap");
			{
				TestClass t1 (new TestClassObj("o7-1"));
				t1.set(new TestClassObj("o7-2"));
			}
			IMDEBUG(DBG_TEST_TITLE, "Createx2 - copy - swap");
			{
				TestClass t1 (new TestClassObj("o7-1"));
				{
					TestClass t2 (t1);
					t1.set(new TestClassObj("o7-2"));
				}
			}
			IMDEBUG(DBG_TEST_TITLE, "Create - reset");
			{
				TestClass t1 (new TestClassObj("o7-1"));
				t1.set(0);
			}
			IMDEBUG(DBG_TEST_TITLE, "Create - Delete - instance");
			{
				Stamina::SharedPtr<TestClassObj2> t1(new TestClassObj2("o8"));
			}
			IMDEBUG(DBG_TEST_TITLE, "Create - copy - (Delete - instance)");
			{
				Stamina::SharedPtr<TestClassObj2> t1(new TestClassObj2("o9-1"));
				IMDEBUG(DBG_TEST, "ustawia nowy obiekt");
				t1 = new TestClassObj2("o9-1");
			}
			
			testResult("Wyciek³o obiektów:", 0, TestClassObj::counter);

			IMDEBUG(DBG_TEST_TITLE, "- Konnekt::KObject }}}");
		}
	}
#endif
}
