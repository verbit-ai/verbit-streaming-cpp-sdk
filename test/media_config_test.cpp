#include <iostream>

#include "media_config_test.h"

using namespace verbit::streaming;

CPPUNIT_TEST_SUITE_REGISTRATION(MediaConfigTest);

void MediaConfigTest::test_default_ctor()
{
	MediaConfig mc = MediaConfig();
	CPPUNIT_ASSERT_MESSAGE("default ctor format", mc.format == "S16LE");
	CPPUNIT_ASSERT_MESSAGE("default ctor sample_rate", mc.sample_rate == 16000);
	CPPUNIT_ASSERT_MESSAGE("default ctor sample_width", mc.sample_width == 2);
	CPPUNIT_ASSERT_MESSAGE("default ctor num_channels", mc.num_channels == 1);
}

void MediaConfigTest::test_url_params()
{
	MediaConfig mc = MediaConfig();
	mc.format = "S8";
	mc.sample_rate = 8000;
	mc.sample_width = 1;
	mc.num_channels = 2;
	CPPUNIT_ASSERT_MESSAGE("url_params", mc.url_params() == "format=S8&sample_rate=8000&sample_width=1&num_channels=2");
}

void MediaConfigTest::test_url_params_non_alnum()
{
	MediaConfig mc = MediaConfig();
	mc.format = "S8+8";
	CPPUNIT_ASSERT_THROW_MESSAGE("url_params with non-alphanumeric chars", mc.url_params(), std::runtime_error);
}
