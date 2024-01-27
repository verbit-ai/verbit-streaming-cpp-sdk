#include <chrono>
#include <fstream>

#include "ws_streaming_client.h"

namespace verbit {
namespace streaming {

WebSocketStreamingClient::WebSocketStreamingClient(std::string access_token) :
	_access_token(access_token),
	_ws_url(WSSC_DEFAULT_WS_URL),
	_max_conn_retry(WSSC_DEFAULT_CONNECTION_RETRY_SECONDS),
	_verify_ssl_cert(true),
	_error_code(0)
{
	if (access_token.empty()) {
		throw std::runtime_error("access token is required");
	}

	// initialize WebSocket++ and set handlers
#if defined(DEBUG)
	_ws_endpoint.set_access_channels(websocketpp::log::alevel::all);
#if !defined(VERBOSE_DEBUG)
	_ws_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
	_ws_endpoint.clear_access_channels(websocketpp::log::alevel::frame_header);
#endif
	_ws_endpoint.set_error_channels(websocketpp::log::elevel::all);
	write_alog("WebSocketStreamingClient ver", WSSC_VERSION);
#else
	_ws_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	_ws_endpoint.clear_error_channels(websocketpp::log::elevel::all);
#endif
	_ws_endpoint.init_asio();
	_ws_endpoint.set_socket_init_handler(bind(&WebSocketStreamingClient::on_socket_init, this, websocketpp::lib::placeholders::_1));
	_ws_endpoint.set_tls_init_handler(bind(&WebSocketStreamingClient::on_tls_init, this, websocketpp::lib::placeholders::_1));
	_ws_endpoint.set_open_handler(bind(&WebSocketStreamingClient::on_open, this, websocketpp::lib::placeholders::_1));
	_ws_endpoint.set_message_handler(bind(&WebSocketStreamingClient::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
	_ws_endpoint.set_close_handler(bind(&WebSocketStreamingClient::on_close, this, websocketpp::lib::placeholders::_1));
	_ws_endpoint.set_fail_handler(bind(&WebSocketStreamingClient::on_fail, this, websocketpp::lib::placeholders::_1));
}

WebSocketStreamingClient::~WebSocketStreamingClient()
{
	write_alog("WebSocketStreamingClient",  "destructor");

	// make the media worker thread exit, if needed, so the following join will not hang
	if (!_state.is_final()) {
		_state.change(ServiceState::state_closing);
	}

	// clean up media worker thread
	if (_media_thread) {
		if (_media_thread->joinable()) {
			_media_thread->join();
		}
		delete _media_thread;
		_media_thread = nullptr;
	}

	write_alog("WebSocketStreamingClient",  "media thread exited");

	// close any previously opened log files
	if (_alog) {
		delete(_alog);
		_alog = nullptr;
	}
	if (_elog) {
		delete(_elog);
		_elog = nullptr;
	}
}

void WebSocketStreamingClient::log_path(const std::string log_path) {
	if (!log_path.empty()) {
		std::string filename;
		// close any previously opened log files
		if (_alog) {
			delete(_alog);
			_alog = nullptr;
		}
		if (_elog) {
			delete(_elog);
			_elog = nullptr;
		}

		int num_start = 0;
		char xbe_log_name[512];
		struct stat sb;
		// find the next filename that doesn't exist yet
		do {
			num_start++;
			snprintf(xbe_log_name, 512, "%s/alog_%d", log_path.c_str(), num_start);
		} while (::stat(xbe_log_name, &sb) == 0);

		// open access log file
		_alog = new std::ofstream(xbe_log_name);
		if (_alog && _alog->is_open()) {
			_ws_endpoint.get_alog().set_ostream(_alog);
			_ws_endpoint.set_access_channels(websocketpp::log::alevel::all);
#if !defined(VERBOSE_DEBUG)
			_ws_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
			_ws_endpoint.clear_access_channels(websocketpp::log::alevel::frame_header);
#endif
			_ws_endpoint.set_error_channels(websocketpp::log::elevel::all);
			write_alog("WebSocketStreamingClient ver", WSSC_VERSION);
		}

		// open error log file
		snprintf(xbe_log_name, 512, "%s/elog_%d", log_path.c_str(), num_start);
		_elog = new std::ofstream(xbe_log_name);
		if (_elog && _elog->is_open()) {
			_ws_endpoint.get_elog().set_ostream(_elog);
			_ws_endpoint.set_error_channels(websocketpp::log::elevel::all);
		}
	}
}

const std::string WebSocketStreamingClient::ws_full_url()
{
	std::string url = _ws_url;
	std::string sep = "?";
	if (url.find('?') != std::string::npos) {
		sep = "&";
	}
	url = url + sep + _media_config.url_params();
	url = url + "&" + _response_types.url_params();
	return url;
}

bool WebSocketStreamingClient::run_stream(MediaGenerator& media_generator)
{
	// construct with default `MediaConfig` and `ResponseType`
	return run_stream(media_generator, MediaConfig(), ResponseType());
}

bool WebSocketStreamingClient::run_stream(MediaGenerator& media_generator, const MediaConfig& media_config, const ResponseType& response_types)
{
	if (_state.get() != ServiceState::state_initial) {
		throw std::runtime_error("retrying is not currently supported");
	}

	// initialize more members
	_media_generator = &media_generator;
	_media_config = media_config;
	_response_types = response_types;

	write_alog("ws_full_url", ws_full_url());

	// connect to the WebSocket server
	_state.change(ServiceState::state_opening);
	websocketpp::lib::error_code ec;
	_ws_con = _ws_endpoint.get_connection(ws_full_url(), ec);
	if (ec) {
		write_alog("get_connection error", ec.message());
		websocketpp::lib::error_code transport_ec = _ws_con->get_transport_ec();
		write_alog("transport-specific get_connection error", transport_ec.message());
		_error_code = _ws_con->get_local_close_code();
		_service_error = _ws_con->get_local_close_reason();
		return false;
	}
	_ws_con->append_header("Authorization", std::string("Bearer ") + _access_token);
	_ws_endpoint.connect(_ws_con);

	write_alog("WebSocket", "connected");

	// start media_generator thread
	_media_thread = new std::thread(&WebSocketStreamingClient::run_media, this);

	// start the ASIO io_service run loop: this doesn't return until the WebSocket closes
	_ws_endpoint.run();

	std::string debug = std::string("run is finished; error_code=") + std::to_string(_error_code);
	write_alog("media", debug);

	return (_error_code == 0);
}

bool WebSocketStreamingClient::stop_stream()
{
	int stateBefore = _state.get();

	write_alog("stop_stream from state",  _state.c_str());

	if (!_state.is_final()) {
		_state.change(ServiceState::state_closing);
	}

	// perform additional steps if the state was state_open
	if (stateBefore == ServiceState::state_open) {
		// allow time for the media worker thread to read and send the last chunk
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// close websocket to make run_stream() return
		write_alog("stop_stream", "closing WebSocket");
		close_ws();
		// wait up to a second for the websocket to close
		_state.wait_for(ServiceState::state_done, std::chrono::milliseconds(1000));
	}

	return (_state.get() == ServiceState::state_done);
}

void WebSocketStreamingClient::run_media()
{
	static ssize_t wssc_bytes_sent = 0L;
	static ssize_t wssc_report_at_bytes = 0L;

	// wait for WebSocket to be open
	while (_state.get() == ServiceState::state_opening) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::string debug = std::string("finished opening; state=") + _state.c_str();
	write_alog("WebSocket", debug);

	// start sending audio chunks
	while ( (_state.get() == ServiceState::state_open) && !_media_generator->finished() ) {
		std::string chunk = _media_generator->get_chunk();
		if (chunk == MediaGenerator::END_OF_FILE) {
			_error_code = AUDIO_TCP_ERROR;
			stop_stream();
		}
		else if (chunk.length() > 0) {
			websocketpp::connection_hdl hdl = _ws_con->get_handle();
			websocketpp::lib::error_code ec;

			_ws_endpoint.send(hdl, chunk, websocketpp::frame::opcode::binary, ec);

			if (ec) {
				static int errorCount = 0;
				errorCount++;
				std::stringstream ec_ss;
				ec_ss << ec;
				write_alog("send audio error count", std::to_string(errorCount));
				write_alog("send audio ec", ec_ss.str());
				write_alog("send audio ec message", ec.message());
				if (errorCount > 10) {
					_error_code = ec.value();
					stop_stream();
				}
			}

			wssc_bytes_sent += chunk.length();
			if (wssc_bytes_sent > wssc_report_at_bytes) {
				std::stringstream media_ss;
				media_ss << "sent chunk " << chunk.length() << " bytes" <<
					" get_buffered_amount() " << _ws_con->get_buffered_amount() <<
					" have sent " << wssc_bytes_sent << " bytes" << std::endl;
				write_alog("media", media_ss.str());
				wssc_report_at_bytes += 500000L;
			}

#if defined(VERBOSE_DEBUG)
			std::stringstream media_ss;
			media_ss << "sent chunk " << chunk.length() << " bytes" <<
				" get_buffered_amount() " << _ws_con->get_buffered_amount() << std::endl;
			write_alog("media", media_ss.str());
#endif
		}
	}

	if ( (_state.get() == ServiceState::state_open) && _media_generator->finished() ) {
		std::string event_eos = "{\"event\":\"EOS\",\"payload\":{}}";

		write_alog("media", "finished");

		_state.change_if(ServiceState::state_closing, ServiceState::state_open, false);

		for (int i = 0; i < 15 && _state.get() != ServiceState::state_done; i++) {
			// the EOS (end of stream) message tells the service we are done
			// sending media, and the order can be finalized; after this point
			// we should see responses with `is_end_of_stream=true`
			// which will cause us to close the WebSocket
			websocketpp::connection_hdl hdl = _ws_con->get_handle();
			websocketpp::lib::error_code ec;

			_ws_endpoint.send(hdl, event_eos, websocketpp::frame::opcode::text, ec);

			if (ec) {
				std::stringstream ec_ss;
				ec_ss << ec;
				write_alog("send eos error on iteration", std::to_string(i));
				write_alog("send eos ec", ec_ss.str());
				write_alog("send eos ec message", ec.message());
			}

			write_alog("media", "sent EOS");
			_state.wait_for(ServiceState::state_done, std::chrono::milliseconds(1000));
		}
		if (_state.get() == ServiceState::state_done) {
			write_alog("WebSocket", "closed due to EOS");
		}
		else {
			write_alog("media", "is_end_of_stream=true not received from service");
			// this will cause run_stream() to exit
			close_ws();
		}
	}
	else {
		std::string debug = std::string("exited loop without finish; state=") + _state.c_str();
		write_alog("media", debug);
	}
}

void WebSocketStreamingClient::close_ws()
{
	write_alog("WebSocket", "closing");
	if (_ws_con)
	{
		websocketpp::connection_hdl hdl = _ws_con->get_handle();
		_ws_endpoint.close(hdl, websocketpp::close::status::going_away, "");
	}
}

// NOTE: this method has no effect and does no logging unless
// _ws_endpoint.set_access_channels() has been called in the constructor
// or in the WebSocketStreamingClient::log_path() method.
void WebSocketStreamingClient::write_alog(std::string tag, std::string message)
{
	std::string line;
	if (tag.empty()) {
		line = message;
	} else {
		line = tag + ": " + message;
	}
	_ws_endpoint.get_alog().write(websocketpp::log::alevel::app, line);
}

void WebSocketStreamingClient::on_socket_init(websocketpp::connection_hdl hdl)
{
	write_alog("WebSocket", "on_socket_init called");
}

wspp_context_ptr WebSocketStreamingClient::on_tls_init(websocketpp::connection_hdl hdl)
{
	write_alog("WebSocket", "on_tls_init called");
	wspp_context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
	if (!_verify_ssl_cert) {
		ctx->set_verify_mode(boost::asio::ssl::verify_none);
	}
	ctx->set_options(boost::asio::ssl::context::default_workarounds |
	                 boost::asio::ssl::context::no_sslv2 |
	                 boost::asio::ssl::context::no_sslv3 |
	                 boost::asio::ssl::context::no_tlsv1 |
	                 boost::asio::ssl::context::single_dh_use);
	return ctx;
}

// NOTE: the `on_fail` handler will only be called during initialization;
// after `on_open` is called, `on_fail` will never be called
void WebSocketStreamingClient::on_fail(websocketpp::connection_hdl hdl)
{
	_state.change_if(ServiceState::state_fail, ServiceState::state_opening, true);

	wspp_client::connection_ptr con = _ws_endpoint.get_con_from_hdl(hdl);

	write_alog("on_fail state", std::to_string(con->get_state()));
	websocketpp::lib::error_code ec = con->get_ec();
	std::stringstream ec_ss;
	ec_ss << ec;
	write_alog("on_fail ec", ec_ss.str());
	write_alog("on_fail ec message", con->get_ec().message());
	// unfortunately this doesn't provide detail like "Connection refused":
	websocketpp::lib::error_code transport_ec = con->get_transport_ec();
	std::stringstream transport_ec_ss;
	transport_ec_ss << transport_ec;
	write_alog("on_fail transport-specific ec", transport_ec_ss.str());
	write_alog("on_fail transport-specific ec message", transport_ec.message());
	write_alog("on_fail local code", std::to_string(con->get_local_close_code()));
	write_alog("on_fail local code", con->get_local_close_reason());
	write_alog("on_fail remote code", std::to_string(con->get_remote_close_code()));
	write_alog("on_fail remote code", con->get_remote_close_reason());
	write_alog("on_fail response message", con->get_response_msg());

	if (con->get_local_close_code() == WS_1006) {  // abnormal WS close
		_error_code = WS_1006;
		_service_error = con->get_local_close_reason();
		// override with specific cases we know:
		if (_service_error == "Invalid HTTP status.") {
			if (con->get_response_msg().substr(0, 12) == "Unauthorized") {
				_error_code = 401;
				_service_error = con->get_response_msg();
			} else if (con->get_response_msg().substr(0, 23) == "Authentication rejected") {
				_error_code = 401;
				_service_error = con->get_response_msg();
			}
		}
	}
}

void WebSocketStreamingClient::on_open(websocketpp::connection_hdl hdl)
{
	_state.change_if(ServiceState::state_open, ServiceState::state_opening, true);
	std::string debug = std::string("on_open called; state=") + _state.c_str();
	write_alog("WebSocket", debug);
}

// anonymous namespace is used here to define translation-unit-local helper methods
// without having to declare them elsewhere
namespace {

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

} // anonymous namespace

void WebSocketStreamingClient::on_message(websocketpp::connection_hdl hdl, wspp_message_ptr msg)
{
	size_t payload_len = msg->get_payload().length();
	std::string debug = std::string("on_message called:")
		+ " frame_type " + _frame_type_str(msg->get_opcode(), msg->get_compressed(), msg->get_fin())
		+ " payload_len " + std::to_string(payload_len);
	write_alog("WebSocket", debug);

	// parse message JSON and deliver to handler
	nlohmann::json message = nlohmann::json::parse(msg->get_payload());
	if (_handler) {
		_handler(this, &message);
	}

	// when all `is_end_of_stream=true` responses have been received, close the WebSocket
	auto it = message.find("response");
	if (it == message.end()) {
		write_alog("", "JSON message received with no 'response' entry");
	} else {
		_response_types.record_eos(message["response"]);
	}
	if (_response_types.is_eos()) {
		close_ws();
	}
}

void WebSocketStreamingClient::on_close(websocketpp::connection_hdl hdl)
{
	_state.change_unless(ServiceState::state_done, ServiceState::state_fail, false);
	std::string debug = std::string("on_close called; state=") + _state.c_str();
	write_alog("WebSocket", debug);

	// check if this close was caused by an error
	wspp_client::connection_ptr con = _ws_endpoint.get_con_from_hdl(hdl);
	websocketpp::lib::error_code ec = con->get_ec();
	if (ec) {
		std::stringstream ec_ss;
		ec_ss << ec;
		write_alog("on_close ec", ec_ss.str());
		write_alog("on_close ec message", ec.message());
		// setting _error_code causes run_stream() to return false
		_error_code = ec.value();
	}
}

} // namespace
} // namespace
