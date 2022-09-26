#include <cppunit/extensions/HelperMacros.h>

#include <verbit/streaming/ws_streaming_client.h>

/**
 * Unit tests for `WebSocketStreamingClient` class.
 */
class WebSocketStreamingClientTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(WebSocketStreamingClientTest);

	CPPUNIT_TEST(test_ctor);
	CPPUNIT_TEST(test_ctor_empty_token);
	CPPUNIT_TEST(test_set_ws_url);
	CPPUNIT_TEST(test_set_ws_url_with_params);
	CPPUNIT_TEST(test_ws_full_url);
	CPPUNIT_TEST(test_ws_full_url_with_params);
	CPPUNIT_TEST(test_set_max_connection_retry);
	CPPUNIT_TEST(test_set_verify_ssl_cert);

	CPPUNIT_TEST_SUITE_END();

public:
	void test_ctor();
	void test_ctor_empty_token();
	void test_set_ws_url();
	void test_set_ws_url_with_params();
	void test_ws_full_url();
	void test_ws_full_url_with_params();
	void test_set_max_connection_retry();
	void test_set_verify_ssl_cert();
};
