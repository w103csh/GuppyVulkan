#ifndef OBJ_DRAW_INST_3D_H
#define OBJ_DRAW_INST_3D_H

#include <memory>

#include "ObjInst3d.h"

class ObjDrawInst3d : public ObjInst3d {
   public:
    ObjDrawInst3d(std::shared_ptr<Instance::Base>& pInstanceData) : ObjInst3d(pInstanceData) {}

    // vkCmdBindVertexBuffers
    inline uint32_t getInstanceFirstBinding() const { return Instance::BINDING; }
    inline uint32_t getInstanceBindingCount() const { return 1; }
    inline const VkBuffer* getInstanceBuffers() const { return &pInstanceData_->BUFFER_INFO.bufferInfo.buffer; }
    inline const VkDeviceSize* getInstanceOffsets() const { return &pInstanceData_->BUFFER_INFO.memoryOffset; }

    // vkCmdDrawIndexed, vkCmdDraw
    inline uint32_t getInstanceCount() const { return pInstanceData_->BUFFER_INFO.count; }
    inline uint32_t getInstanceFirstInstance() const { return 0; }
};

#endif  // !OBJ_DRAW_INST_3D_H