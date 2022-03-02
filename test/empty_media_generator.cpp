#include "empty_media_generator.h"

namespace verbit {
namespace streaming {

const std::string EmptyMediaGenerator::get_chunk()
{
	return std::string();
}

} // namespace
} // namespace
