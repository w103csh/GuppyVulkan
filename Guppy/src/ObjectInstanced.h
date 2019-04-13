#ifndef INSTANCED_OBJECT_H
#define INSTANCED_OBJECT_H

#include <memory>

#include "Instance.h"

class ObjectInstanced {
   public:
    ObjectInstanced(std::shared_ptr<Instance::Base>& pInstanceData) : pInstanceData_(pInstanceData) {}

    // vkCmdBindVertexBuffers
    inline uint32_t getInstanceFirstBinding() const { return Instance::BINDING; }
    inline uint32_t getInstanceBindingCount() const { return 1; }
    inline const VkBuffer* getInstanceBuffers() const { return &pInstanceData_->BUFFER_INFO.bufferInfo.buffer; }
    inline const VkDeviceSize* getInstanceOffsets() const { return &pInstanceData_->BUFFER_INFO.memoryOffset; }

    // vkCmdDrawIndexed, vkCmdDraw
    virtual inline uint32_t getInstanceCount() const { return pInstanceData_->BUFFER_INFO.count; }
    virtual inline uint32_t getInstanceFirstInstance() const { return 0; }

    inline const Buffer::Info& getInstanceDataInfo() { return pInstanceData_->BUFFER_INFO; }

   protected:
    std::shared_ptr<Instance::Base> pInstanceData_;
};

#endif  // !INSTANCED_OBJECT_H