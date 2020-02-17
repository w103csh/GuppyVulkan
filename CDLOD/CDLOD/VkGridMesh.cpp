//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
//      This concept came from DxGridMesh
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#include "VkGridMesh.h"

#include <vector>

#include <Common/Helpers.h>

VkGridMesh::VkGridMesh(const Context& context)
    : m_pContext(&context),
      m_indexBuffer(),
      m_vertexBuffer(),
      m_dimension(),
      m_indexEndTL(),
      m_indexEndTR(),
      m_indexEndBL(),
      m_indexEndBR() {}

VkGridMesh::~VkGridMesh(void) { OnDestroyDevice(); }

void VkGridMesh::SetDimensions(int dim) { m_dimension = dim; }

vk::Result VkGridMesh::OnCreateDevice(LoadingResource& ldgRes) {
    if (m_pContext == nullptr) return vk::Result::eNotReady;
    if (m_dimension == 0) return vk::Result::eSuccess;

    std::string name = "VkGridMesh(" + std::to_string(m_dimension) + ")";

    const int gridDim = m_dimension;

    int totalVertices = (gridDim + 1) * (gridDim + 1);
    assert(totalVertices <= 65535);
    std::vector<glm::vec3> vertices(totalVertices);

    int totalIndices = gridDim * gridDim * 2 * 3;
    std::vector<IndexBufferType> indices(totalIndices);

    int vertDim = gridDim + 1;

    {
        // Make a grid of (gridDim+1) * (gridDim+1) vertices

        for (int y = 0; y < vertDim; y++)
            for (int x = 0; x < vertDim; x++) vertices[x + vertDim * y] = {x / (float)(gridDim), y / (float)(gridDim), 0};

        BufferResource stgRes = {};
        m_pContext->createBuffer(ldgRes.transferCmd,
                                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                 sizeof(glm::vec3) * vertices.size(), name.c_str(), stgRes, m_vertexBuffer, vertices.data());
        ldgRes.stgResources.push_back(std::move(stgRes));
    }

    {
        // Make indices for the gridDim * gridDim triangle grid, but make it as a combination of 4 subquads so that they
        // can be rendered separately when needed!

        int index = 0;

        int halfd = (vertDim / 2);
        int fulld = gridDim;

        // Top left part
        for (int y = 0; y < halfd; y++) {
            for (int x = 0; x < halfd; x++) {
                indices[index++] = (unsigned short)(x + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * (y + 1));
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
            }
        }
        m_indexEndTL = index;

        // Top right part
        for (int y = 0; y < halfd; y++) {
            for (int x = halfd; x < fulld; x++) {
                indices[index++] = (unsigned short)(x + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * (y + 1));
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
            }
        }
        m_indexEndTR = index;

        // Bottom left part
        for (int y = halfd; y < fulld; y++) {
            for (int x = 0; x < halfd; x++) {
                indices[index++] = (unsigned short)(x + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * (y + 1));
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
            }
        }
        m_indexEndBL = index;

        // Bottom right part
        for (int y = halfd; y < fulld; y++) {
            for (int x = halfd; x < fulld; x++) {
                indices[index++] = (unsigned short)(x + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
                indices[index++] = (unsigned short)((x + 1) + vertDim * y);
                indices[index++] = (unsigned short)((x + 1) + vertDim * (y + 1));
                indices[index++] = (unsigned short)(x + vertDim * (y + 1));
            }
        }
        m_indexEndBR = index;

        BufferResource stgRes = {};
        m_pContext->createBuffer(
            ldgRes.transferCmd, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            sizeof(IndexBufferType) * vertices.size(), name.c_str(), stgRes, m_indexBuffer, indices.data());
        ldgRes.stgResources.push_back(std::move(stgRes));
    }

    return vk::Result::eSuccess;
}

void VkGridMesh::OnDestroyDevice() {
    if (m_pContext == nullptr) return;
    m_pContext->destroyBuffer(m_indexBuffer);
    m_pContext->destroyBuffer(m_vertexBuffer);
    m_indexEndTL = m_indexEndTR = m_indexEndBL = m_indexEndBR = 0;
}

void VkGridMesh::CreateBuffers(LoadingResource& ldgRes) {
    OnDestroyDevice();
    OnCreateDevice(ldgRes);
}
