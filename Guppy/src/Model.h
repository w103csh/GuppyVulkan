#pragma once

#include <vector>

#include "Constants.h"
#include "Plane.h"
#include "Vertex.h"

const std::string CHALET_MODEL_PATH = "models/chalet.obj";
const std::string CHALET_TEXTURE_PATH = "textures/chalet.jpg";

class Model {
   public:
    inline Vertex* getVetexData() { return m_vertices.data(); }

    inline VkDeviceSize getVertexBufferSize() {
        VkDeviceSize p_bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
        return p_bufferSize;
    }

    inline VB_INDEX_TYPE* getIndexData() { return m_indices.data(); }

    inline uint32_t getIndexSize() { return static_cast<uint32_t>(m_indices.size()); }

    inline VkDeviceSize getIndexBufferSize() {
        VkDeviceSize p_bufferSize = sizeof(m_indices[0]) * m_indices.size();
        return p_bufferSize;
    }

    void addToModel(const std::vector<Vertex>& vertices, const std::vector<VB_INDEX_TYPE>& indices);
    void addToModel(Plane&& p);
    void loadAxes();
    void loadDefault();
    void loadChalet();

   private:
    VB_INDEX_TYPE m_maxIndex = UINT32_MAX;
    std::vector<Vertex> m_vertices;
    std::vector<VB_INDEX_TYPE> m_indices;
    unsigned long long m_linesOffset;
};