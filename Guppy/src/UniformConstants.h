#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <map>
#include <set>
#include <utility>

#include "Types.h"

enum class PIPELINE : uint32_t;
enum class RENDER_PASS : uint32_t;

namespace Uniform {

using index = uint8_t;
const auto UNIFORM_MAX_COUNT = UINT8_MAX;

// Uniform buffer offsets to pass types
using offsets = std::set<uint32_t>;
using offsetsMapValue = std::map<offsets, std::set<RENDER_PASS>>;

// Descriptor/pipeline to offsets/pass types
using offsetsMapKey = std::pair<DESCRIPTOR, PIPELINE>;
using offsetsKeyValue = std::pair<const offsetsMapKey, offsetsMapValue>;
using offsetsMap = std::map<offsetsMapKey, offsetsMapValue>;

// Constants for ease of use
extern const offsets OFFSET_SINGLE_SET;
extern const offsets OFFSET_ALL_SET;
extern const offsets OFFSET_DONT_CARE;
extern const std::set<RENDER_PASS> RENDER_PASS_ALL_SET;
extern const offsetsMapValue OFFSET_SINGLE_DEFAULT_MAP;
extern const offsetsMapValue OFFSET_ALL_DEFAULT_MAP;

}  // namespace Uniform

#endif  //! UNIFORM_CONSTANTS_H