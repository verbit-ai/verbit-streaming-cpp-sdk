#include <iostream>

#include <nlohmann/json.hpp>

namespace verbit {
namespace streaming {

/**
 * Class to describe the response types returned by the Verbit Transcribe Streaming service.
 */
class ResponseType {
public:
	static const int Transcript = 0x01;
	static const int Captions = 0x02;

	/// Construct a new default response type descriptor.
	/// In this form, the default response types (`Captions`) will be used.
	constexpr ResponseType() : _types(Captions), _eos_types(0) { }

	/// Construct a new response type descriptor using the given response type(s).
	/// The types are bits, which can be combined with logical-or to select the desired types, _e.g._
	/// `ResponseType(ResponseType::Transcript | ResponseType::Captions)`
	///
	/// \param types the response type(s) which should be returned by the service
	constexpr ResponseType(int types) : _types(types), _eos_types(0) { }

	/// Construct a new response type descriptor by response type name(s).
	/// The types are bits, and the comma-separated string can contain multiple desired types, _e.g._
	/// `"Transcript,Captions"`
	///
	/// \param types the response type(s) which should be returned by the service
	ResponseType(std::string const types);

	/// Get response type(s) in integer (bits) form.
	constexpr int types() { return _types; }

	/// Get response type(s) in string form.
	/// This returns a comma-separated list of response type(s), _e.g._ `"Transcript,Captions"`.
	std::string to_string() const;

	/// Has end-of-stream been received for all response types?
	constexpr bool is_eos() { return (_types & _eos_types) == _types; }

	/// Does this descriptor include the given response type?
	///
	/// \param type the response type to check
	constexpr bool includes(int type) { return (_types & type) ? true : false; }

	/// Record whether end-of-stream has been received for the given response.
	/// The `response` object should contain `type` and `is_end_of_stream` elements.
	///
	/// \param response the response to check for end-of-stream
	void record_eos(nlohmann::json &response);

	std::string url_params();

private:
	int _types;
	int _eos_types;
};

} // namespace
} // namespace

std::ostream& operator<<(std::ostream&, const verbit::streaming::ResponseType&);
