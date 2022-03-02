#include <iostream>
#include <fstream>
#include <string>

#include <verbit/streaming/media_generator.h>

/// audio chunk size, in milliseconds
#define CHUNK_DURATION_MS 100

// assuming input file is a 16-bit mono 16000Hz PCM Wave file:
#define CHUNK_BYTES ((CHUNK_DURATION_MS * 2 * 1 * 16000) / 1000)

/**
 * Class implementing a WAV file media generator.
 *
 * This class currently expects a WAV file with S16LE 16-bit mono 16kHz PCM format.
 */
class WAVMediaGenerator : public verbit::streaming::MediaGenerator
{
public:
	/// Construct a media generator from a WAV file.
	WAVMediaGenerator(const std::string& filename);

	~WAVMediaGenerator();

	/// Return the next chunk of media bytes from the WAV file.
	///
	/// The returned `string` is binary data, up to `CHUNK_BYTES` in length.
	/// When the end of the WAV file is reached, returns an empty `string`.
	const std::string get_chunk();

	/// Returns `true` when the end of the WAV file is reached.
	///
	/// **NOTE** When this method returns `true`, the caller will
	/// send the end-of-stream event. This will close out the
	/// Verbit order, after which no more media may be sent.
	bool finished() { return _file.eof(); }

private:
	const std::string& _filename;
	std::ifstream _file;
};
