#ifndef UNIFORM_OFFSETS_MANAGER_H
#define UNIFORM_OFFSETS_MANAGER_H

#include "Constants.h"

namespace Uniform {

class OffsetsManager {
   public:
    enum class ADD_TYPE {
        Pipeline,
        RenderPass,
    };

    OffsetsManager();

    void addOffsets(const offsetsMap& map, const ADD_TYPE&& addType);

    inline const auto& getOffsetMap() const { return offsetsMap_; }

    void validateInitialMap();
    void validateAddType(const offsetsMap& map, const ADD_TYPE& addType);

   private:
    offsetsMap offsetsMap_;
};

}  // namespace Uniform

#endif  //! UNIFORM_OFFSETS_MANAGER_H