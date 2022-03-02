#include <cppunit/extensions/HelperMacros.h>

#include <verbit/streaming/response_type.h>

/**
 * Unit tests for `ResponseType` class.
 */
class ResponseTypeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ResponseTypeTest);

	CPPUNIT_TEST(test_default_ctor);
	CPPUNIT_TEST(test_int_ctor);
	CPPUNIT_TEST(test_string_ctor);
	CPPUNIT_TEST(test_string_ctor_invalid);
	CPPUNIT_TEST(test_to_string);
	CPPUNIT_TEST(test_to_string_none);
	CPPUNIT_TEST(test_to_string_invalid);
	CPPUNIT_TEST(test_url_params);
	CPPUNIT_TEST(test_eos);

	CPPUNIT_TEST_SUITE_END();

public:
	void test_default_ctor();
	void test_int_ctor();
	void test_string_ctor();
	void test_string_ctor_invalid();
	void test_to_string();
	void test_to_string_none();
	void test_to_string_invalid();
	void test_url_params();
	void test_eos();
};
