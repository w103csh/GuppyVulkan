//#pragma once
//
//#include <vector>
//
//#include "Constants.h"
//#include "Plane.h"
//#include "Vertex.h"
//
//const std::string CHALET_MODEL_PATH = "..\\..\\..\\data\\chalet.obj";
//const std::string CHALET_TEXTURE_PATH = "..\\..\\..\\images\\chalet.jpg";
//
//class Model {
//   public:
//    inline Vertex* getVertexData() { return vertices_.data(); }
//
//    inline uint32_t getVertexCount() { return vertices_.size(); }
//
//    inline VkDeviceSize getVertexBufferSize() {
//        VkDeviceSize p_bufferSize = sizeof(vertices_[0]) * vertices_.size();
//        return p_bufferSize;
//    }
//
//    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
//
//    inline uint32_t getIndexSize() { return static_cast<uint32_t>(indices_.size()); }
//
//    inline VkDeviceSize getIndexBufferSize() {
//        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
//        return p_bufferSize;
//    }
//
//    void addToModel(const std::vector<Vertex>& vertices, const std::vector<VB_INDEX_TYPE>& indices);
//    void addToModel(Plane&& p);
//    void loadAxes();
//    void loadDefault();
//    void loadChalet();
//    uint32_t m_linesCount = 0;
//
//   private:
//    VB_INDEX_TYPE max_index_ = UINT32_MAX;
//    std::vector<Vertex> vertices_;
//    std::vector<VB_INDEX_TYPE> indices_;
//    unsigned long long lines_offset_ = 0;
//};