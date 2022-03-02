#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <sysexits.h>

#include <nlohmann/json.hpp>

#include <verbit/streaming/ws_streaming_client.h>

#include "wav_media_generator.h"

using namespace verbit::streaming;

// TODO trap `SIGTERM` and have it set `should_stop` - 2nd `SIGTERM` hard exits

void usage(char* argv0)
{
	std::cerr << "Usage: example_client [ -k ] [ -u URL ] file.wav" << std::endl;
	std::cerr << "  -?, -h, --help        this help message" << std::endl;
	std::cerr << "  -k, --insecure        skip server SSL certificate verification" << std::endl;
	std::cerr << "  -u URL, --ws-url=URL  server WebSocket URL" << std::endl;
}

bool config_from_options(int argc, char** argv, WebSocketStreamingClient &client, std::string &wavfile)
{
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help",     no_argument,       0, 'h' },
			{"insecure", no_argument,       0, 'k' },
			{"ws-url",   required_argument, 0, 'u' },
			{0,          0,                 0, 0   }
		};
		int c = getopt_long(argc, argv, "?hku:", long_options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case '?':
		case 'h':
			usage(argv[0]);
			return false;
		case 'k':
			client.verify_ssl_cert(false);
			break;
		case 'u':
			client.ws_url(optarg);
			break;
		}
	}
	if (optind >= argc) {
		std::cerr << argv[0] << ": WAV filename is required" << std::endl;
		usage(argv[0]);
		return false;
	}
	wavfile = argv[optind];
	return true;
}

void on_response(WebSocketStreamingClient* client, nlohmann::json* response)
{
	auto alternatives = (*response)["response"]["alternatives"];
	std::string transcript = alternatives[0]["transcript"].get<std::string>();
	std::cout << transcript << std::endl;
}

int main(int argc, char** argv)
{
	// construct Verbit streaming client
	char* env_token = getenv("VERBIT_WS_TOKEN");
	const std::string access_token = env_token ? env_token : "<your_access_token>";
	WebSocketStreamingClient client {access_token};

	// process command-line options
	std::string wavfile;
	if (!config_from_options(argc, argv, client, wavfile)) {
		return EX_USAGE;
	}

	// set handler for client to deliver responses from service
	// (see documentation for how to set a class method as a handler)
	client.set_response_handler(&on_response);

	// construct media generator
	WAVMediaGenerator media_gen {wavfile};

	// send the audio stream and receive responses
	if (!client.run_stream(media_gen)) {
		std::cerr << "error " << client.error_code() << ": " << client.service_error() << std::endl;
		return EX_SOFTWARE;
	} else {
		return EX_OK;
	}
}
