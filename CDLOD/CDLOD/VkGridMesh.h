////////////////////////////////////////////////////////////////////////
//// Modifications copyright(C) 2021 Colin Hughes<colin.s.hughes @gmail.com>
////      This concept came from DxGridMesh
//// -------------------------------
//// Copyright (C) 2009 - Filip Strugar.
//// Distributed under the zlib License (see readme.txt)
////////////////////////////////////////////////////////////////////////

#ifndef _VK_GRID_MESH_H_
#define _VK_GRID_MESH_H_

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

#include <Common/Context.h>

class VkGridMesh {
   private:
    const Context* m_pContext;
    BufferResource m_indexBuffer;
    BufferResource m_vertexBuffer;
    int m_dimension;
    int m_indexEndTL;
    int m_indexEndTR;
    int m_indexEndBL;
    int m_indexEndBR;

   public:
    using VertexBufferType = glm::vec2;

    VkGridMesh(const Context& context);
    ~VkGridMesh(void);
    //
    void destroy() { OnDestroyDevice(); }
    //
    void SetDimensions(int dim);
    int GetDimensions() const { return m_dimension; }
    //
    const auto& GetIndexBuffer() const { return m_indexBuffer; }
    const auto& GetVertexBuffer() const { return m_vertexBuffer; }
    int GetIndexEndTL() const { return m_indexEndTL; }
    int GetIndexEndTR() const { return m_indexEndTR; }
    int GetIndexEndBL() const { return m_indexEndBL; }
    int GetIndexEndBR() const { return m_indexEndBR; }
    //
    void CreateBuffers(LoadingResource& ldgRes);
    //
   private:
    //
    virtual vk::Result OnCreateDevice(LoadingResource& ldgRes);
    virtual void OnDestroyDevice();
};

#endif  // !_VK_GRID_MESH_H_