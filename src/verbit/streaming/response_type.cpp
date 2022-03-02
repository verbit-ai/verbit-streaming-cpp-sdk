#include "response_type.h"

namespace
{
	std::string _bool_to_string(bool b)
	{
		return b ? std::string("True") : std::string("False");
	}
}

namespace verbit {
namespace streaming {

static int _string_to_response_types(std::string const types)
{
	if (types == "Transcript") {
		return ResponseType::Transcript;
	} else if (types == "Captions") {
		return ResponseType::Captions;
	} else if (types == "Transcript,Captions") {
		return ResponseType::Transcript | ResponseType::Captions;
	} else if (types == "Captions,Transcript") {
		return ResponseType::Transcript | ResponseType::Captions;
	} else {
		throw std::runtime_error("unsupported ResponseType value");
	}
}

ResponseType::ResponseType(std::string const types)
	: _types(_string_to_response_types(types)), _eos_types(0)
{
}

std::string ResponseType::to_string() const
{
	switch (_types) {
	case 0x00:
		return std::string("None");
	case Transcript:
		return std::string("Transcript");
	case Captions:
		return std::string("Captions");
	case Transcript | Captions:
		return std::string("Transcript,Captions");
	default:
		throw std::runtime_error("unsupported ResponseType value");
	}
}

std::string ResponseType::url_params()
{
	return std::string("get_transcript=") + _bool_to_string(includes(ResponseType::Transcript)) +
		"&get_captions=" + _bool_to_string(includes(ResponseType::Captions));
}

void ResponseType::record_eos(nlohmann::json &response)
{
	bool is_eos = false;
	auto it = response.find("is_end_of_stream");
	if (it != response.end()) {
		nlohmann::json value = *it;
		if (!value.is_null()) {
			is_eos = value.get<bool>();
		}
	}
	it = response.find("type");
	if (is_eos && (it != response.end())) {
		nlohmann::json value = *it;
		if (!value.is_null()) {
			std::string type = value.get<std::string>();
			type[0] = toupper(type[0]);
			_eos_types |= _string_to_response_types(type);
#if defined(DEBUG)
			ResponseType _seen_types {_eos_types};
			std::cout << "have seen EOS response types: " << _seen_types.to_string() << std::endl;
#endif
		}
	}
}

std::ostream& operator<<(std::ostream& ost, const ResponseType& rt)
{
	ost << "ResponseType(" << rt.to_string() << ")";
	return ost;
}

} // namespace
} // namespace
