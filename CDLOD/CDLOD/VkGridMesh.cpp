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

VkGridMesh::~VkGridMesh(void) {}

void VkGridMesh::SetDimensions(int dim) { m_dimension = dim; }

vk::Result VkGridMesh::OnCreateDevice(LoadingResource& ldgRes) {
    if (m_pContext == nullptr) return vk::Result::eNotReady;
    if (m_dimension == 0) return vk::Result::eSuccess;

    /* Examples:
     *  2x2
     *    (0,0)     (0,1)
     *      0----1----2
     *      | TL | TR |     o Numbers are the vertex order
     *      3----4----5     o Indices are in TL->TR->BL->BR order
     *      | BL | BR |     o Note: The TL is actually the bottom left coordiante wise
     *      6----7----8
     *    (0,1)     (1,1)
     *
     *      Index buffer: 0,1,3,1,4,3,1,2,4,2,5,4,3,4,6...
     *
     *  4x4
     *   0----1----2----3----4
     *   | TL | TL | TR | TR |
     *   5----6----7----8----9
     *   | TL | TL | TR | TR |
     *   10---11---12---13---14
     *   | BL | BL | BR | BR |
     *   15---16---17---18---19
     *   | BL | BL | BR | BR |
     *   20---21---22---23---24
     *
     *      Index buffer: 0,1,5,1,6,5,1,2,6,2,7,6,5,6,10...
     *  CH
     */

    std::string dimStr = std::to_string(m_dimension);
    std::string name = "VkGridMesh(" + dimStr + "x" + dimStr + ")";

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
            sizeof(IndexBufferType) * indices.size(), name.c_str(), stgRes, m_indexBuffer, indices.data());
        ldgRes.stgResources.push_back(std::move(stgRes));
    }

    return vk::Result::eSuccess;
}

void VkGridMesh::OnDestroyDevice() {
    if (m_pContext != nullptr) {
        m_pContext->destroyBuffer(m_indexBuffer);
        m_pContext->destroyBuffer(m_vertexBuffer);
        m_indexEndTL = m_indexEndTR = m_indexEndBL = m_indexEndBR = 0;
    }
}

void VkGridMesh::CreateBuffers(LoadingResource& ldgRes) {
    OnDestroyDevice();
    OnCreateDevice(ldgRes);
}
