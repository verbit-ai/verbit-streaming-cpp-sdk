#pragma once

#include <string>

namespace verbit {
namespace streaming {

/**
 * Abstract base class defining the interface for a media generator.
 */
class MediaGenerator
{
public:
	virtual ~MediaGenerator() {}

	/// Return the next chunk of media bytes.
	///
	/// The returned `string` is binary data.
	/// If there are no media bytes waiting, returns an empty `string`.
	///
	/// **NOTE** This method will be called in a tight loop.
	/// To avoid high CPU load, it is expected to either:
	/// 1. Block until more media bytes are available.
	/// 2. Wait a short while (_e.g._ 100ms) before returning empty.
	virtual const std::string get_chunk() = 0;

	/// Has all media now been sent?
	///
	/// **NOTE** When this method returns `true`, the caller will
	/// send the end-of-stream event. This will close out the
	/// Verbit order, after which no more media may be sent.
	virtual bool finished() = 0;
};

} // namespace
} // namespace
