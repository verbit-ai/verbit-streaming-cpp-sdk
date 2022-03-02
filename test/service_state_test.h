#include <cppunit/extensions/HelperMacros.h>

#include <verbit/streaming/service_state.h>

/**
 * Unit tests for `ServiceState` class.
 */
class ServiceStateTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ServiceStateTest);

	CPPUNIT_TEST(test_default_ctor);
	CPPUNIT_TEST(test_c_str);
	CPPUNIT_TEST(test_change);
	CPPUNIT_TEST(test_change_if);
	CPPUNIT_TEST(test_change_if_except);
	CPPUNIT_TEST(test_change_unless);
	CPPUNIT_TEST(test_change_unless_except);

	CPPUNIT_TEST_SUITE_END();

public:
	void test_default_ctor();
	void test_c_str();
	void test_change();
	void test_change_if();
	void test_change_if_except();
	void test_change_unless();
	void test_change_unless_except();
};
