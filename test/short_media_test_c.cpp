#include <iostream>
#include <sysexits.h>

#include <nlohmann/json.hpp>

#include <verbit/streaming/ws_streaming_client.h>

#include "../examples/wav_media_generator.h"

#define TEST_WS_URL    "wss://localhost:9002"
#define TEST_WAV_FILE  "test-files/thats-good.wav"
#define DUMP_FILENAME  "/tmp/wss_test_server.bin"

#define EXPECTED_N_RESPONSES  2
#define EXPECTED_FINAL_TEXT   "I saw 44346 bytes. "
int n_responses = 0;
std::string final_text;

using namespace verbit::streaming;

bool compare_received_bytes()
{
	// open `test_server` dump file
	std::string _filename = DUMP_FILENAME;
	std::ifstream _file;
	_file.exceptions(std::ifstream::badbit);
	_file.open(_filename, std::ios::binary);
	if (_file.fail()) {
		throw std::runtime_error(std::string("can't open ") + _filename + ": " + strerror(errno));
	}

	// compare
	WAVMediaGenerator media_gen {TEST_WAV_FILE};
	char buf[CHUNK_BYTES];
	bool success = true;
	size_t i = 0, len;
	while (!media_gen.finished()) {
		std::string wav_chunk = media_gen.get_chunk();
		len = wav_chunk.length();

		std::string dump_chunk;
		if (len > 0) {
			_file.read(buf, len);
			std::streamsize count = _file.gcount();
			dump_chunk = std::string(buf, count);
		} else {
			dump_chunk = std::string();
		}

		if (dump_chunk != wav_chunk) {
			std::cout << "FAILED mismatch in test_server received bytes @ byte " << i << " len=" << len << std::endl;
			success = false;
			break;
		} else {
			i += len;
		}
	}

	// wrap up
	_file.close();
	return success;
}

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
	WAVMediaGenerator media_gen {TEST_WAV_FILE};
	if (!client.run_stream(media_gen)) {
		std::cout << "FAILED error " << client.error_code() << ": " << client.service_error() << std::endl;
	} else if (n_responses != EXPECTED_N_RESPONSES) {
		std::cout << "FAILED expected n_responses=" << EXPECTED_N_RESPONSES << " actual n_responses=" << n_responses << std::endl;
	} else if (final_text != EXPECTED_FINAL_TEXT) {
		std::cout << "FAILED expected final_text=\"" << EXPECTED_FINAL_TEXT << "\" actual final_text=\"" << final_text << "\"" << std::endl;
	} else if (!compare_received_bytes()) {
		// emits its own FAILED message
	} else {
		std::cout << "OK (2 tests)" << std::endl;
		return EX_OK;
	}
	return EX_SOFTWARE;
}
