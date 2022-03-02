#include <string>

#include <verbit/streaming/media_generator.h>

namespace verbit {
namespace streaming {

/**
 * Class implementing an empty file media generator.
 * Used only as a placeholder for certain unit tests.
 */
class EmptyMediaGenerator : public MediaGenerator
{
public:
	const std::string get_chunk();
	bool finished() { return true; }
};

} // namespace
} // namespace
