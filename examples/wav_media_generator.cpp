#include <chrono>
#include <thread>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "wav_media_generator.h"

using namespace verbit::streaming;

WAVMediaGenerator::WAVMediaGenerator(const std::string& filename) :
	_filename(filename)
{
	// only throw exception if stream is compromised
	_file.exceptions(std::ifstream::badbit);

	// open file
	_file.open(_filename, std::ios::binary);
	if (_file.fail()) {
		// TODO - will throw exception for file-not-found/permission-denied (w/caller exiting EX_NOINPUT)
		throw std::runtime_error(std::string("can't open ") + _filename + ": " + strerror(errno));
	}
#if defined(DEBUG)
	std::cout << "reading from " << _filename << std::endl;
#endif

	// skip RIFF WAVE header
	// TODO header length can vary; must parse it to find skip length
	_file.seekg(78, std::ios_base::cur);
}

WAVMediaGenerator::~WAVMediaGenerator()
{
	_file.close();
}

const std::string WAVMediaGenerator::get_chunk()
{
	if (_file.eof()) {
		return std::string();
	}

	// read next chunk (binary) - might be less than what we asked for
	char buf[CHUNK_BYTES];
	_file.read(buf, CHUNK_BYTES);
	std::streamsize count = _file.gcount();
#if defined(VERBOSE_DEBUG)
	std::cout << "read " << count << " bytes" << std::endl;
#endif

	// simulate realtime playback-rate by sleeping
	std::this_thread::sleep_for(std::chrono::milliseconds(CHUNK_DURATION_MS));

	// return chunk as `string`
	return std::string(buf, count);
}
