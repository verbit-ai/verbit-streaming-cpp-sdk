#include <chrono>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <signal.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <uuid/uuid.h>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#define LATENCY 250
//#define ATMOSPHERICS
//#define NO_LABELS
//#define INT_SPEAKER_ID
//#define FAIL_CONNECT_SOMETIMES

typedef websocketpp::server<websocketpp::config::asio_tls> wspp_server;
typedef websocketpp::config::asio::message_type::ptr wspp_message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> wspp_context_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// NOTE this breaks if more than one client connects to us simultaneously
#define DUMP_FILENAME "/tmp/wss_test_server.bin"
std::chrono::time_point<std::chrono::system_clock> media_start;
size_t seen_bytes;
size_t sent_resp_bytes;
std::ofstream dump_file;
bool translation_service;

#define PIDFILE "/tmp/wss_test_server.pid"

#if defined(SSL_CERT_HAS_PASSWORD)
std::string get_password()
{
	return "<the-SSL-cert-password>";
}
#endif

wspp_context_ptr on_tls_init(wspp_server* s, websocketpp::connection_hdl hdl)
{
	std::cout << "on_tls_init called" << std::endl;
	namespace asio = websocketpp::lib::asio;
	wspp_context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);

	try {
		ctx->set_options(asio::ssl::context::default_workarounds |
				 asio::ssl::context::no_sslv2 |
				 asio::ssl::context::no_sslv3 |
				 asio::ssl::context::no_tlsv1 |
				 asio::ssl::context::single_dh_use);

		ctx->set_verify_mode(asio::ssl::verify_none);

#if defined(SSL_CERT_HAS_PASSWORD)
		ctx->set_password_callback(bind(&get_password));
#endif
		ctx->use_certificate_chain_file("test-files/server.pem");
		ctx->use_private_key_file("test-files/server.pem", asio::ssl::context::pem);

		std::string ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
		if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
			std::cerr << "error setting cipher list" << std::endl;
		}
	} catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << std::endl;
	}
	return ctx;
}

// Simple way to test that `Authorization` header was provided
bool on_validate(wspp_server* s, websocketpp::connection_hdl hdl) {
	wspp_server::connection_ptr con = s->get_con_from_hdl(hdl);

#if defined(FAIL_CONNECT_SOMETIMES)
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	if ((seconds & 0x7) > 1) {
		con->set_status(websocketpp::http::status_code::request_timeout);
		std::cout << "on_validate set HTTP 408 (simulated connect failure)" << std::endl;
		return false;
	}
#endif

	std::string auth_hdr(con->get_request_header("Authorization"));
	std::string sanitized_hdr = auth_hdr;
	if (auth_hdr.length() >= 11) {
		sanitized_hdr = auth_hdr.substr(0, 9) +
			"[" + std::to_string(auth_hdr.length() - 11) + "]" +
			auth_hdr.substr(auth_hdr.length() - 2, 2);
	}
	std::cout << "on_validate called: Authorization: " << sanitized_hdr << std::endl;
	if (auth_hdr.length() < 40) {  // JWT tokens are long
		con->set_status(websocketpp::http::status_code::unauthorized);
		return false;
	}
	if (auth_hdr.find("LANG") == std::string::npos) {
		translation_service = false;
	} else {
		translation_service = true;
	}
	return true;
}

void on_open(wspp_server* s, websocketpp::connection_hdl hdl) {
	wspp_server::connection_ptr con = s->get_con_from_hdl(hdl);
	websocketpp::uri_ptr uri = con->get_uri();
	std::cout << "on_open called, uri = " << uri->str() << std::endl;
	std::string query = uri->get_query();
	std::cout << "on_open query = " << query << std::endl;
	std::string req_body = con->get_request_body();
	std::cout << "on_open request_body = " << req_body << std::endl;
	seen_bytes = 0;
	sent_resp_bytes = 0;

	// (re)open file
	if (dump_file.is_open()) {
		dump_file.close();
		::unlink(DUMP_FILENAME);
	}
	dump_file.exceptions(std::ofstream::badbit);
	dump_file.open(DUMP_FILENAME, std::ios::binary);
	if (dump_file.fail()) {
		throw std::runtime_error(std::string("can't open ") + DUMP_FILENAME + ": " + strerror(errno));
	}
}

std::string _frame_type_str(websocketpp::frame::opcode::value opcode, bool compressed, bool fin)
{
	std::string opcode_type;
	switch (opcode) {
	case 0x00:
		opcode_type = "cont-frame";
		break;
	case 0x01:
		opcode_type = "text-frame";
		break;
	case 0x02:
		opcode_type = "binary-frame";
		break;
	case 0x08:
		opcode_type = "ctl:close-frame";
		break;
	case 0x09:
		opcode_type = "ctl:ping-frame";
		break;
	case 0x0a:
		opcode_type = "ctl:pong-frame";
		break;
	default:
		opcode_type = std::string("unknown-") + std::to_string((int)opcode);
		break;
	}
	if (compressed) {
		opcode_type += ":compressed";
	} else {
		opcode_type += ":uncompressed";
	}
	if (fin) {
		opcode_type += ":fin";
	}
	return opcode_type;
}

typedef std::vector<std::string> stringVector;

stringVector tokenize(std::string line)
{
	stringVector tokens;
	std::stringstream ss(line);
	std::string token;
	while (getline(ss, token, ' ')) {
		tokens.push_back(token);
	}
	return tokens;
}

std::string response_items(std::string transcript, std::string speaker_id)
{
	stringVector words = tokenize(transcript);
	std::string r_items;
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - media_start);
	long tick = milliseconds.count() - LATENCY;
	for (const std::string &word: words) {
		if (!r_items.empty()) {
			r_items += ",";
		}
		double start_t = ((double)tick) / 1000.0;
		double end_t = ((double)tick + 7) / 1000.0;
		if (word == "." || word == "," || word == "?" || word == "!") {
			r_items += std::string("{\"kind\":\"punct\",");
		} else {
			r_items += std::string("{\"kind\":\"text\",");
		}
		r_items += (std::string("\"value\":\"") + word + "\",");
#ifdef INT_SPEAKER_ID
		if (speaker_id.empty()) {
			r_items += (std::string("\"speaker_id\":0,"));
		} else {
			r_items += (std::string("\"speaker_id\":\"") + speaker_id + "\",");
		}
#else
		r_items += (std::string("\"speaker_id\":\"") + speaker_id + "\",");
#endif
		char tstr[128];
		sprintf(tstr, "%.3f", start_t);
		r_items += (std::string("\"start\":") + tstr + ",");
		sprintf(tstr, "%.3f", end_t);
		r_items += (std::string("\"end\":") + tstr + "}");
		tick += 20;
	}
	return r_items;
}

#ifdef ATMOSPHERICS
int didApplause = false;
#endif

std::string response_json(bool eos, std::string lang)
{
	std::string transcript;
	std::string language_code;
	// NB punct (final period) must be separate token
	if (lang == "es-ES") {
		transcript = std::string(eos ? "Vi " : "He visto ") +
			std::to_string(seen_bytes) + " bytes . ";
		language_code = "es-ES";
	} else {
		transcript = std::string(eos ? "I saw " : "I've seen ") +
			std::to_string(seen_bytes) + " bytes . ";
		language_code = "en-US";
	}
	std::string eos_s = eos ? "true" : "false";
	uuid_t uuid;
	uuid_generate_random(uuid);
	char* uuid_p = new char[256];
	uuid_unparse(uuid, uuid_p);
	std::string speaker_uuid;
#ifndef ATMOSPHERICS
	if ((seen_bytes % 288000) < 96000) {
		speaker_uuid = "c6eb6f2b-f85b-478f-af8a-a21b00000001";
	} else if ((seen_bytes % 288000) < 198000) {
		speaker_uuid = "c6eb6f2b-f85b-478f-af8a-a21b00000002";
	} else {
#ifdef INT_SPEAKER_ID
		speaker_uuid = "";
#else
		speaker_uuid = "c6eb6f2b-f85b-478f-af8a-a21b00000003";
#endif
	}
#else
	speaker_uuid = "c6eb6f2b-f85b-478f-af8a-a21b00000001";
	if ((seen_bytes % 288000) < 224000) {
		didApplause = false;
	} else if (!didApplause) {
		transcript = "[APPLAUSE] ";
		didApplause = true;
	}
#endif
	std::string items = response_items(transcript.substr(0, transcript.length() - 1), speaker_uuid);
	std::string service_type;
	if (translation_service) {
		service_type = "translation";
	} else {
		service_type = "transcription";
	}
	std::string json = std::string("{\"response\":{") +
		"\"id\":\"" + uuid_p + "\"," +
		"\"type\":\"captions\"," +
		"\"service_type\":\"" + service_type + "\"," +
		"\"language_code\":\"" + language_code + "\"," +
		"\"is_final\":true,\"is_end_of_stream\":" + eos_s + "," +
		"\"speakers\":[" +
#ifndef NO_LABELS
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000001\",\"label\":\"First Host\"}," +
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000002\",\"label\":\"Second Host\"}," +
#ifdef INT_SPEAKER_ID
		"{\"id\":0,\"label\":\"\"}" +
#else
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000003\",\"label\":\"Speaker 3\"}" +  // unidentified
#endif
#else
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000001\",\"label\":\"\"}," +
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000002\",\"label\":\"\"}," +
#ifdef INT_SPEAKER_ID
		"{\"id\":0,\"label\":\"\"}" +
#else
		"{\"id\":\"c6eb6f2b-f85b-478f-af8a-a21b00000003\",\"label\":\"\"}" +
#endif
#endif
		"]," +
		"\"alternatives\":[{" +
		"\"transcript\":\"" + transcript + "\"," +
		"\"items\":[" + items + "]" +
		"}]}}";
	delete uuid_p;
	sent_resp_bytes = seen_bytes;
	return json;
}

void on_message_text(wspp_server* s, websocketpp::connection_hdl hdl, wspp_server::message_ptr msg) {
	size_t payload_len = msg->get_payload().length();
	std::cout << "on_message (text) called: frame_type " << _frame_type_str(msg->get_opcode(), msg->get_compressed(), msg->get_fin())
		<< " payload_len " << std::to_string(payload_len) << std::endl;
	if (payload_len > 0) {
		// assume this is the special "EOS" JSON event message;
		// reply with a response that has `is_end_of_stream=true`
		std::string json = response_json(true, "en-US");
		s->send(hdl, json, websocketpp::frame::opcode::text);
		std::cout << "on_message (text) replied w/text: " << json << std::endl;
	}
}

void on_message_binary(wspp_server* s, websocketpp::connection_hdl hdl, wspp_server::message_ptr msg) {
	if (seen_bytes == 0) {
		media_start = std::chrono::system_clock::now();
	}
	size_t payload_len = msg->get_payload().length();
	seen_bytes += payload_len;
#if defined(VERBOSE_DEBUG)
	std::cout << "on_message (binary) called: frame_type " << _frame_type_str(msg->get_opcode(), msg->get_compressed(), msg->get_fin())
		<< " payload_len " << std::to_string(payload_len)
		<< " seen_bytes " << std::to_string(seen_bytes) << std::endl;
#endif
	if (payload_len > 0) {
		dump_file.write(msg->get_payload().c_str(), payload_len);
	}

	if ((seen_bytes - sent_resp_bytes) >= 32000) {  // 1 sec
		// simulate delay from producing captions:
		std::this_thread::sleep_for(std::chrono::milliseconds(LATENCY));
		try {
			std::string json;
			if (translation_service) {
				json = response_json(false, "es-ES");
				s->send(hdl, json, websocketpp::frame::opcode::text);
				std::cout << "on_message (binary) replied w/text (es-ES): " << json << std::endl;
			}
			json = response_json(false, "en-US");
			s->send(hdl, json, websocketpp::frame::opcode::text);
			std::cout << "on_message (binary) replied w/text (en-US): " << json << std::endl;
		} catch (websocketpp::exception const & e) {
			std::cerr << "on_message (binary) send failed: " << "(" << e.what() << ")" << std::endl;
		}
	}
}

void on_message(wspp_server* s, websocketpp::connection_hdl hdl, wspp_server::message_ptr msg) {
	if (msg->get_opcode() == websocketpp::frame::opcode::text) {
		on_message_text(s, hdl, msg);
	} else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
		on_message_binary(s, hdl, msg);
	} else {
		std::cerr << "on_message called w/unsupported frame_type "
			<< _frame_type_str(msg->get_opcode(), msg->get_compressed(), msg->get_fin())
			<< std::endl;
	}
}

void on_close(wspp_server* s, websocketpp::connection_hdl hdl) {
	std::cout << "on_close called" << std::endl;
	dump_file.close();
}

void pidlock(char* arg0) {
	pid_t pid = getpid();
	std::string pid_s = std::to_string(pid);
	/* TODO should read and check for stale pid */
	int fd = ::open(PIDFILE, O_CREAT|O_EXCL|O_WRONLY, 0644);
	if (fd < 0) {
		if (errno == EEXIST) {
			std::cerr << arg0 << ": " << PIDFILE << " already exists; exiting" << std::endl;
			exit(EX_UNAVAILABLE);
		} else {
			std::cerr << arg0 << ": error creating " << PIDFILE << ": " << strerror(errno) << std::endl;
			exit(EX_OSERR);
		}
	}
	::write(fd, pid_s.c_str(), pid_s.length());
	::close(fd);
}

void pidunlock() {
	unlink(PIDFILE);
}

void sighandler(int sig) {
	if (sig == 1) {
		std::cerr << "caught SIGHUP; ignoring" << std::endl;
	} else {
		std::cerr << "caught signal " << sig << "; exiting" << std::endl;
		pidunlock();
		exit(0);
	}
}

int main(int argc, char** argv)
{
	wspp_server test_server;
	int port = 9002;

	if (argc > 1) {
		port = atoi(argv[1]);
	}
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGTERM, sighandler);
	pidlock(argv[0]);
	try {
#if defined(DEBUG)
		test_server.set_access_channels(websocketpp::log::alevel::all);
#if !defined(VERBOSE_DEBUG)
		test_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
		test_server.clear_access_channels(websocketpp::log::alevel::frame_header);
#endif
		test_server.set_error_channels(websocketpp::log::elevel::all);
#else
		test_server.clear_access_channels(websocketpp::log::alevel::all);
		test_server.clear_error_channels(websocketpp::log::elevel::all);
#endif
		test_server.init_asio();
		test_server.set_tls_init_handler(bind(&on_tls_init, &test_server, ::_1));
		test_server.set_validate_handler(bind(&on_validate, &test_server, ::_1));
		test_server.set_open_handler(bind(&on_open, &test_server, ::_1));
		test_server.set_message_handler(bind(&on_message, &test_server, ::_1, ::_2));
		test_server.set_close_handler(bind(&on_close, &test_server, ::_1));
		std::cout << "listen on port " << port << std::endl;
		test_server.listen(port);
		test_server.start_accept();
		test_server.run();
	} catch (websocketpp::exception const & e) {
		std::cerr << e.what() << std::endl;
	} catch (std::exception& e) {
		std::cerr << "other exception: " << e.what() << std::endl;
	}
	pidunlock();
}
