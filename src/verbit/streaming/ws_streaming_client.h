#pragma once

#include <thread>

#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <verbit/streaming/media_config.h>
#include <verbit/streaming/media_generator.h>
#include <verbit/streaming/response_type.h>
#include <verbit/streaming/service_state.h>
#include <verbit/streaming/version.h>

#define WSSC_DEFAULT_WS_URL "wss://speech.verbit.co/ws"
#define WSSC_DEFAULT_CONNECTION_RETRY_SECONDS 60.0

namespace verbit {
namespace streaming {

typedef websocketpp::client<websocketpp::config::asio_tls_client> wspp_client;
typedef websocketpp::config::asio_tls_client::message_type::ptr wspp_message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> wspp_context_ptr;

class WebSocketStreamingClient;
typedef std::function<void(WebSocketStreamingClient*, nlohmann::json*)> wssc_response_handler;

/**
 * Class to connect to the Verbit Transcribe Streaming service.
 */
class WebSocketStreamingClient
{
public:
	/// Construct a new streaming client.
	///
	/// \param access_token credential required to access the service
	WebSocketStreamingClient(std::string access_token);

	/// Destruct the streaming client.
	~WebSocketStreamingClient();

	/// Return the WebSocket base URL.
	const std::string ws_url() { return _ws_url; }

	/// Set the WebSocket base URL.
	void ws_url(const std::string ws_url) { _ws_url = ws_url; }

	/// Return the number of seconds to retry connecting to the WebSocket server before giving up.
	/// Retry uses random exponential backoff.
	constexpr double max_connection_retry_seconds() { return _max_conn_retry; }

	/// Set the number of seconds to retry connecting to the WebSocket server before giving up.
	/// Retry uses random exponential backoff.
	void max_connection_retry_seconds(double seconds) { _max_conn_retry = seconds; }

	/// Return the SSL certificate verification behavior.
	constexpr bool verify_ssl_cert() { return _verify_ssl_cert; }

	/// Set the SSL certificate verification behavior.
	/// Default `true`. Set this to `false` to skip verifying the server SSL certificate.
	/// This has security implications, and should only be used for testing!
	void verify_ssl_cert(bool verify_ssl_cert) { _verify_ssl_cert = verify_ssl_cert; }

	/// Set the handler used to deliver responses from the service.
	///
	/// The `handler` function should take two arguments (and return `void`):
	/// - `WebSocketStreamingClient*` — pointer to this client
	/// - `nlohmann::json*` — pointer to a JSON object containing the response
	///
	/// The JSON schema for the responses can be found in
	/// [examples/responses/schema.md](https://github.com/verbit-ai/verbit-streaming-python-sdk/blob/main/examples/responses/schema.md).
	///
	/// The `handler` can be a C function, or can be bound to the class method of an object as follows:
	/// ```
	/// #include <functional>
	///
	/// client.set_response_handler(std::bind(&MyClass::method, &object, std::placeholders::_1, std::placeholders::_2));
	/// ```
	void set_response_handler(wssc_response_handler handler) { _handler = handler; }

	/// Return the numeric error code.
	///
	/// \return
	/// - 0 for no error
	/// - 400-599 for HTTP status codes (_e.g._ 401 for Unauthorized)
	/// - 1000+ for WebSocket errors (_e.g._ 1006 for abnormal close)
	constexpr int error_code() { return _error_code; }

	/// Return the service-level error message.
	const std::string service_error() { return _service_error; }

	/// Start the WebSocket stream using the given media generator.
	///
	/// In this form, the default media config (S16LE 16kHz mono PCM) and response types (Captions) will be used.
	///
	/// **NOTE** This method will not return until `media_generator` reports finished or an error is encountered.
	///
	/// \return `false` if an error was encountered; use error methods for details
	bool run_stream(MediaGenerator& media_generator);

	/// Start the WebSocket stream using the given media generator, media config, and desired response types.
	///
	/// **NOTE** This method will not return until `media_generator` reports finished or an error is encountered.
	///
	/// \return `false` if an error was encountered; use error methods for details
	bool run_stream(MediaGenerator& media_generator, const MediaConfig& media_config, const ResponseType& response_types);

private:
	std::string _access_token;
	std::string _ws_url;
	double _max_conn_retry;
	bool _verify_ssl_cert;
	wssc_response_handler _handler = nullptr;
	MediaGenerator* _media_generator = nullptr;
	std::thread* _media_thread = nullptr;
	MediaConfig _media_config;
	ResponseType _response_types;
	ServiceState _state;
	wspp_client _ws_endpoint;
	wspp_client::connection_ptr _ws_con = nullptr;
	int _error_code;
	std::string _service_error;

	std::string ws_full_url()
	{
		return _ws_url + "?" + _media_config.url_params() +
			"&" + _response_types.url_params();
	}

	void run_media();
	void close_ws();
	void write_alog(std::string tag, std::string message);

	void on_socket_init(websocketpp::connection_hdl hdl);
	wspp_context_ptr on_tls_init(websocketpp::connection_hdl hdl);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, wspp_message_ptr msg);
	void on_close(websocketpp::connection_hdl hdl);
};

} // namespace
} // namespace
