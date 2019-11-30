/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OBJ_DRAW_INST_3D_H
#define OBJ_DRAW_INST_3D_H

#include <memory>

#include "Obj3dInst.h"

namespace Obj3d {
class InstanceDraw : public Obj3d::Instance {
   public:
    InstanceDraw(std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData) : Obj3d::Instance(pInstanceData) {}

    // vkCmdBindVertexBuffers
    inline uint32_t getInstanceFirstBinding() const { return 1; }
    inline uint32_t getInstanceBindingCount() const { return 1; }
    inline const VkBuffer* getInstanceBuffers() const { return &pInstObj3d_->BUFFER_INFO.bufferInfo.buffer; }
    inline const VkDeviceSize* getInstanceOffsets() const { return &pInstObj3d_->BUFFER_INFO.memoryOffset; }

    // vkCmdDrawIndexed, vkCmdDraw
    inline uint32_t getInstanceCount() const { return pInstObj3d_->BUFFER_INFO.count; }
    inline uint32_t getInstanceFirstInstance() const { return 0; }
};
}  // namespace Obj3d

#endif  // !OBJ_DRAW_INST_3D_H