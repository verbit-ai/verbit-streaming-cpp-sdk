#include <iostream>

namespace verbit {
namespace streaming {

/**
 * Structure to describe the media format sent by the client.
 */
struct MediaConfig {
	std::string format = "S16LE";  ///< media format, _e.g._ signed 16-bit little-endian PCM
	int sample_rate = 16000;       ///< sample rate, in Hz
	int sample_width = 2;          ///< sample width, in bytes
	int num_channels = 1;          ///< number of channels

	std::string url_params();
};

} // namespace
} // namespace

std::ostream& operator<<(std::ostream&, const verbit::streaming::MediaConfig&);
