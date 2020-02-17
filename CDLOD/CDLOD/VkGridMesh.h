////////////////////////////////////////////////////////////////////////
//// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
////      This concept came from DxGridMesh
//// -------------------------------
//// Copyright (C) 2009 - Filip Strugar.
//// Distributed under the zlib License (see readme.txt)
////////////////////////////////////////////////////////////////////////

#ifndef VK_GRID_MESH_H
#define VK_GRID_MESH_H

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
    VkGridMesh(const Context& context);
    ~VkGridMesh(void);
    //
    void SetDimensions(int dim);
    int GetDimensions() const { return m_dimension; }
    //
    const auto GetIndexBuffer() const { return m_indexBuffer; }
    const auto GetVertexBuffer() const { return m_vertexBuffer; }
    int GetIndexEndTL() const { return m_indexEndTL; }
    int GetIndexEndTR() const { return m_indexEndTR; }
    int GetIndexEndBL() const { return m_indexEndBL; }
    int GetIndexEndBR() const { return m_indexEndBR; }
    //
    void CreateBuffers(LoadingResource& pLdgRes);
    //
   private:
    //
    virtual vk::Result OnCreateDevice(LoadingResource& ldgRes);
    virtual void OnDestroyDevice();
};

#endif  // !VK_GRID_MESH_H