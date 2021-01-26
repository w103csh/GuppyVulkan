/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OBJ_INST_3D_H
#define OBJ_INST_3D_H

#include <memory>

#include "Instance.h"
#include "Obj3d.h"

namespace Obj3d {

/* This class is kind of weird. The idea is that the instance data can be shared, so instead of
 *  an object actually being an Instance::Obj3d::Base it is instead this object which implements the
 *  Obj3d::Interface and wraps the actual data. This allows the instance data to be created outside
 *  of the object itself and potentially be changed separately as well. I'm not sure if I like this
 *  though...
 */
class Instance : public Obj3d::Interface {
   public:
    Instance(std::shared_ptr<::Instance::Obj3d::Base>& pInstObj3d) : pInstObj3d_(pInstObj3d) {
        assert(pInstObj3d_ != nullptr);
    }

    inline uint32_t getModelCount() { return pInstObj3d_->BUFFER_INFO.count; }
    inline const Buffer::Info& getInstanceDataInfo() { return pInstObj3d_->BUFFER_INFO; }

    // Obj3d::Interface
    inline glm::vec3 worldToLocal(const glm::vec3& v, const bool isPosition = false,
                                  const uint32_t index = 0) const override {
        return pInstObj3d_->worldToLocal(v, std::forward<const bool>(isPosition), std::forward<const uint32_t>(index));
    }
    inline glm::vec3 getWorldSpaceDirection(const glm::vec3& d = FORWARD_VECTOR, const uint32_t index = 0) const override {
        return pInstObj3d_->getWorldSpaceDirection(d, std::forward<const uint32_t>(index));
    }
    inline glm::vec3 getWorldSpacePosition(const glm::vec3& p = {}, const uint32_t index = 0) const override {
        return pInstObj3d_->getWorldSpacePosition(p, std::forward<const uint32_t>(index));
    }
    inline BoundingBoxMinMax getBoundingBoxMinMax(const bool transform = true, const uint32_t index = 0) const override {
        return pInstObj3d_->getBoundingBoxMinMax(std::forward<const bool>(transform), std::forward<const uint32_t>(index));
    }
    inline bool testBoundingBox(const Ray& ray, const float& tMin, const bool useDirection = true,
                                const uint32_t index = 0) const override {
        return pInstObj3d_->testBoundingBox(ray, tMin, std::forward<const bool>(useDirection),
                                            std::forward<const uint32_t>(index));
    }
    inline BoundingBox getBoundingBox(const uint32_t index = 0) const override {
        return pInstObj3d_->getBoundingBox(std::forward<const uint32_t>(index));
    }
    inline const glm::mat4& getModel(const uint32_t index = 0) const override {
        return pInstObj3d_->getModel(std::forward<const uint32_t>(index));
    }
    inline void transform(const glm::mat4 t, const uint32_t index = 0) override {
        return pInstObj3d_->transform(std::forward<const glm::mat4>(t), std::forward<const uint32_t>(index));
    }
    inline void putOnTop(const BoundingBoxMinMax& boundingBox,
                         const uint32_t index = ::Instance::Obj3d::MODEL_ALL) override {
        return pInstObj3d_->putOnTop(boundingBox, std::forward<const uint32_t>(index));
    }
    inline void setModel(const glm::mat4 m, const uint32_t index = 0) override {
        pInstObj3d_->setModel(std::forward<const glm::mat4>(m), std::forward<const uint32_t>(index));
    }

   protected:
    std::shared_ptr<::Instance::Obj3d::Base> pInstObj3d_;
};

}  // namespace Obj3d

#endif  // !OBJ_INST_3D_H