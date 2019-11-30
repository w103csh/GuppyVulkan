/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UNIFORM_OFFSETS_MANAGER_H
#define UNIFORM_OFFSETS_MANAGER_H

#include "ConstantsAll.h"

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

    void initializeMap();
    void validateAddType(const offsetsMap& map, const ADD_TYPE& addType);

   private:
    offsetsMap offsetsMap_;
};

}  // namespace Uniform

#endif  //! UNIFORM_OFFSETS_MANAGER_H