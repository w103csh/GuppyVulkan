#ifndef OBJ_INST_3D_H
#define OBJ_INST_3D_H

#include <memory>

#include "Instance.h"
#include "Obj3d.h"

class ObjInst3d : public Obj3d {
   public:
    ObjInst3d(std::shared_ptr<Instance::Base>& pInstanceData) : pInstanceData_(pInstanceData) {}

    // OBJ3D
    inline uint32_t getModelCount() override { return pInstanceData_->BUFFER_INFO.count; }
    void putOnTop(const BoundingBoxMinMax& inBoundingBoxMinMax, uint32_t index = Instance::MODEL_ALL) override;
    inline void transform(const glm::mat4 t, uint32_t index = 0) override {
        pInstanceData_->transform(std::forward<const glm::mat4>(t), std::forward<uint32_t>(index));
    }

    inline const Buffer::Info& getInstanceDataInfo() { return pInstanceData_->BUFFER_INFO; }

   protected:
    // OBJ3D
    inline const glm::mat4& model(uint32_t index = 0) const override { return pInstanceData_->model(index); }

    std::shared_ptr<Instance::Base> pInstanceData_;
};

#endif  // !OBJ_INST_3D_H