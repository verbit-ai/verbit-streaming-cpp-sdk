#include <cppunit/extensions/HelperMacros.h>

#include <verbit/streaming/media_config.h>

/**
 * Unit tests for `MediaConfig` class.
 */
class MediaConfigTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MediaConfigTest);

	CPPUNIT_TEST(test_default_ctor);
	CPPUNIT_TEST(test_url_params);
	CPPUNIT_TEST(test_url_params_non_alnum);

	CPPUNIT_TEST_SUITE_END();

public:
	void test_default_ctor();
	void test_url_params();
	void test_url_params_non_alnum();
};
