#include <algorithm>
#include <string>

#include "media_config.h"

namespace
{
	// h/t <https://stackoverflow.com/a/2926983/291754>
	bool _string_isalnum(const std::string &s)
	{
		    return find_if(s.begin(), s.end(), [](char c) { return !std::isalnum(c); }) == s.end();
	}
}


namespace verbit {
namespace streaming {

std::string MediaConfig::url_params()
{
	// this is the only element which could have characters that need to be URL-encoded
	if (!_string_isalnum(format)) {
		throw std::runtime_error("non-alphanumeric characters in format");
	}
	return std::string("format=") + format +
		"&sample_rate=" + std::to_string(sample_rate) +
		"&sample_width=" + std::to_string(sample_width) +
		"&num_channels=" + std::to_string(num_channels);
}

std::ostream& operator<<(std::ostream& ost, const MediaConfig& mc)
{
	ost << "MediaConfig(format=" << mc.format
	    << " sample_rate=" << mc.sample_rate
	    << " sample_width=" << mc.sample_width
	    << " num_channels=" << mc.num_channels << ")";
	return ost;
}

} // namespace
} // namespace
