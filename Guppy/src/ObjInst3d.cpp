
#include "ObjInst3d.h"

void ObjInst3d::putOnTop(const BoundingBoxMinMax& inBoundingBoxMinMax, uint32_t index) {
    if (index != Instance::MODEL_ALL) {
        Obj3d::putOnTop(inBoundingBoxMinMax, std::forward<uint32_t>(index));
    } else {
        for (index = 0; index < pInstanceData_->BUFFER_INFO.count; index++) {
            auto myBoundingBoxMinMax = getBoundingBoxMinMax(true, index);
            auto t = translateToTop(inBoundingBoxMinMax, myBoundingBoxMinMax);
            transform(t, index);
        }
    }
}
