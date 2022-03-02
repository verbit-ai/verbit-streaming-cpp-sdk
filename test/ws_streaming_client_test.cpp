#include <iostream>

#include "ws_streaming_client_test.h"
#include "empty_media_generator.h"

using namespace verbit::streaming;

CPPUNIT_TEST_SUITE_REGISTRATION(WebSocketStreamingClientTest);

void WebSocketStreamingClientTest::test_ctor()
{
	std::string access_token = "grimblepritz";
	WebSocketStreamingClient client {access_token};
	CPPUNIT_ASSERT_MESSAGE("ctor ws_url", client.ws_url() == WSSC_DEFAULT_WS_URL);
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("ctor max_connection_retry", WSSC_DEFAULT_CONNECTION_RETRY_SECONDS, client.max_connection_retry_seconds(), 0.001);
	CPPUNIT_ASSERT_MESSAGE("ctor verify_ssl_cert", client.verify_ssl_cert() == true);
}

void WebSocketStreamingClientTest::test_ctor_empty_token()
{
	std::string access_token = "";
	CPPUNIT_ASSERT_THROW_MESSAGE("ctor with empty token", new WebSocketStreamingClient(access_token), std::runtime_error);
}

void WebSocketStreamingClientTest::test_set_ws_url()
{
	std::string access_token = "blurfldyick";
	WebSocketStreamingClient client {access_token};
	client.ws_url("wss://speech-qa.verbit.co/ws/test");
	CPPUNIT_ASSERT_MESSAGE("set ws_url", client.ws_url() == "wss://speech-qa.verbit.co/ws/test");
}

void WebSocketStreamingClientTest::test_set_max_connection_retry()
{
	std::string access_token = "foo-bar-baz-qux";
	WebSocketStreamingClient client {access_token};
	client.max_connection_retry_seconds(45.6);
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("set_max_connection_retry max_connection_retry_seconds", 45.6, client.max_connection_retry_seconds(), 0.001);
}

void WebSocketStreamingClientTest::test_set_verify_ssl_cert()
{
	std::string access_token = "xyzzy-plugh";
	WebSocketStreamingClient client {access_token};
	client.verify_ssl_cert(false);
	CPPUNIT_ASSERT_MESSAGE("get verify_ssl_cert", client.verify_ssl_cert() == false);
}
