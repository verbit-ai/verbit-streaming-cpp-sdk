#include <iostream>
#include <sysexits.h>

#include <nlohmann/json.hpp>

#include <verbit/streaming/ws_streaming_client.h>

#include "empty_media_generator.h"

#define TEST_WS_URL "wss://localhost:9002"

#define EXPECTED_N_RESPONSES  1
#define EXPECTED_FINAL_TEXT   "I saw 0 bytes. "
int n_responses = 0;
std::string final_text;

using namespace verbit::streaming;

void on_response(WebSocketStreamingClient* client, nlohmann::json* response)
{
	n_responses++;
	auto is_eos = (*response)["response"]["is_end_of_stream"];
	if (is_eos.get<bool>()) {
		auto alternatives = (*response)["response"]["alternatives"];
		final_text = alternatives[0]["transcript"].get<std::string>();
	}
}

int main(int argc, char** argv)
{
	const std::string access_token = "a-token-longer-than-40-chars-a-token-longer-than-40-chars";
	WebSocketStreamingClient client {access_token};
	client.ws_url(TEST_WS_URL);
	client.verify_ssl_cert(false);
	client.set_response_handler(&on_response);
	EmptyMediaGenerator media_gen;
	if (!client.run_stream(media_gen)) {
		std::cout << "FAILED error " << client.error_code() << ": " << client.service_error() << std::endl;
	} else if (n_responses != EXPECTED_N_RESPONSES) {
		std::cout << "FAILED expected n_responses=" << EXPECTED_N_RESPONSES << " actual n_responses=" << n_responses << std::endl;
	} else if (final_text != EXPECTED_FINAL_TEXT) {
		std::cout << "FAILED expected final_text=\"" << EXPECTED_FINAL_TEXT << "\" actual final_text=\"" << final_text << "\"" << std::endl;
	} else {
		std::cout << "OK (2 tests)" << std::endl;
		return EX_OK;
	}
	return EX_SOFTWARE;
}
