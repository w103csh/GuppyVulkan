
#ifndef MESH_H
#define MESH_H

#include <future>
#include <glm/glm.hpp>
#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Helpers.h"
#include "LoadingResourceHandler.h"
#include "Material.h"
#include "Shell.h"
#include "Object3d.h"
#include "PipelineHandler.h"
#include "Texture.h"

// **********************
//  Mesh
// **********************

typedef struct MeshCreateInfo {
    std::string markerName = "";
    glm::mat4 model = glm::mat4(1.0f);
    size_t offset = 0;
    bool pickable = true;
    std::unique_ptr<Material> pMaterial = nullptr;
} MeshCreateInfo;

class Mesh : public Object3d {
   public:
    typedef enum FLAGS {
        POLY = 0x00000001,
        LINE = 0x00000002,
        // THROUGH 0x00000008
    } FLAGS;

    Mesh(MeshCreateInfo* pCreateInfo);
    /*  THIS IS SUPER IMPORTANT BECAUSE SCENE HAS A VECTOR OF POLYMORPHIC UNIQUE_PTRs OF THIS CLASS.
        IF THIS IS REMOVED THE DESTRUCTOR HERE WILL BE CALLED INSTEAD OF THE DERIVED DESTRUCTOR.
        IT MIGHT JUST BE EASIER/SMARTER TO GET RID OF POLYMORPHISM AND DROP THE POINTERS. */
    virtual ~Mesh() = default;

    // GETTERS
    inline std::string getMarkerName() const { return markerName_; }
    inline size_t getOffset() const { return offset_; }
    inline STATUS getStatus() const { return status_; }
    inline PIPELINE_TYPE getTopologyType() const { return pipelineType_; }
    inline Vertex::TYPE getVertexType() const { return vertexType_; }
    inline const Material& getMaterial() const { return std::cref(*pMaterial_); }
    // inline MeshCreateInfo getCreateInfo() const { return {markerName_, model_, offset_, pickable_, nullptr}; }

    inline void setStatusPendingBuffers() {
        assert(status_ == STATUS::PENDING || status_ == STATUS::PENDING_TEXTURE);
        status_ = STATUS::PENDING_BUFFERS;
    }

    // INIT
    void setSceneData(size_t offset);

    // LOADING
    virtual std::future<Mesh*> load(const Shell::Context& ctx, std::function<void(Mesh*)> callbacak = nullptr);
    virtual void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes);

    // VERTEX
    virtual Vertex::Complete getVertexComplete(size_t index) = 0;
    virtual void addVertex(const Vertex::Complete& v, int32_t index = -1) = 0;
    virtual inline uint32_t getVertexCount() const = 0;  // TODO: this shouldn't be public

    // INDEX
    inline void addIndices(std::vector<VB_INDEX_TYPE>& is) {
        for (auto i : is) indices_.push_back(i);
    }
    inline void addIndex(VB_INDEX_TYPE index) { indices_.push_back(index); }

    // FACE
    virtual bool selectFace(const Ray& ray, Face& face) const { return false; };  //= 0;

    // DRAWING
    void drawInline(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                    const VkDescriptorSet& descSet) const;
    // TODO: this shouldn't be virtual... its only for textures
    virtual void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                               const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets,
                               size_t frameIndex, const VkCommandBufferInheritanceInfo& inheritanceInfo,
                               const VkViewport& viewport, const VkRect2D& scissor) const {};

    virtual void destroy(const VkDevice& dev);

   protected:
    // LOADING
    Mesh* async_load(const Shell::Context& ctx, std::function<void(Mesh*)> callbacak = nullptr);

    // VERTEX
    void loadVertexBuffers(const VkDevice& dev);
    virtual inline const void* getVertexData() const = 0;
    virtual inline VkDeviceSize getVertexBufferSize() const = 0;

    // INDEX
    inline VB_INDEX_TYPE* getIndexData() { return indices_.data(); }
    inline uint32_t getIndexSize() const { return static_cast<uint32_t>(indices_.size()); }
    inline VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(indices_[0]) * indices_.size();
        return p_bufferSize;
    }

    // create info
    std::string markerName_;
    size_t offset_;
    bool pickable_;
    std::unique_ptr<Material> pMaterial_;
    // derived class specific
    FlagBits flags_;
    Vertex::TYPE vertexType_;
    PIPELINE_TYPE pipelineType_;
    //
    STATUS status_;
    BufferResource vertex_res_;
    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource index_res_;
    std::unique_ptr<LoadingResources> pLdgRes_;

   private:
    void createVertexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res);
    void createIndexBufferData(const VkDevice& dev, const VkCommandBuffer& cmd, BufferResource& stg_res);
};

// **********************
// ColorMesh
// **********************

class ColorMesh : public Mesh {
   public:
    ColorMesh(MeshCreateInfo* pCreateInfo);

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) override {
        assert(index < vertices_.size());
        return {vertices_[index]};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getColorVertex());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getColorVertex();
        }
        updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Color) * vertices_.size();
        return p_bufferSize;
    }

    // FACE
    bool selectFace(const Ray& ray, Face& face) const override {
        // Declare some variables that will be reused
        float a, b, c;
        float d, e, f;
        // float g, h, i;
        float j, k, l;

        float ei_hf;
        float gf_di;
        float dh_eg;
        float ak_jb;
        float jc_al;
        float bl_kc;

        float beta, gamma, t, M;

        std::array<glm::vec4, 2> localRay = {glm::vec4(ray.e, 1.0f), glm::vec4(ray.d, 1.0f)};
        worldToLocal(localRay);

        float t0 = 0.0f, t1 = glm::distance(localRay[0], localRay[1]);  // TODO: like this???

        bool faceFound = false;
        for (size_t n = 0; n < indices_.size(); n += 3) {
            // face indices
            const auto& idx0 = indices_[n];
            const auto& idx1 = indices_[n + 1];
            const auto& idx2 = indices_[n + 2];
            // face vertex positions (faces are only triangles for now)
            const auto& pa = vertices_[idx0].position;
            const auto& pb = vertices_[idx1].position;
            const auto& pc = vertices_[idx2].position;

            /*  This is solved using barycentric coordinates, and
                Cramer's rule.

                e + td = a + beta(b-a) + gamma(c-a)

                |xa-xb   xa-xc   xd| |b| = |xa-xe|
                |ya-yb   ya-yc   yd| |g| = |ya-ye|
                |za-zb   za-zc   zd| |t| = |za-ze|

                |a   d   g| |beta | = |j|
                |b   e   h| |gamma| = |k|
                |c   f   i| |t    | = |l|
                


                M =       a(ei-hf) + b(gf-di) + c(dh-eg)
                t =     (f(ak-jb) + e(jc-al) + d(bl-kc)) / -M
                gamma = (i(ak-jb) + h(jc-al) + g(bl-kc)) /  M
                beta =  (j(ei-hf) + k(gf-di) + l(dh-eg)) /  M
            */

            a = pa.x - pb.x;
            b = pa.y - pb.y;
            c = pa.z - pb.z;
            d = pa.x - pc.x;
            e = pa.y - pc.y;
            f = pa.z - pc.z;
            const auto& g = localRay[1].x;
            const auto& h = localRay[1].y;
            const auto& i = localRay[1].z;
            j = pa.x - localRay[0].x;
            k = pa.y - localRay[0].y;
            l = pa.z - localRay[0].z;

            ei_hf = (e * i) - (h * f);
            gf_di = (g * f) - (d * i);
            dh_eg = (d * h) - (e * g);
            ak_jb = (a * k) - (j * b);
            jc_al = (j * c) - (a * l);
            bl_kc = (b * l) - (k * c);

            glm::vec3 c1 = {a, b, c};
            glm::vec3 c2 = {d, e, f};
            glm::vec3 c3 = {g, h, i};
            glm::vec3 c4 = {j, k, l};

            glm::mat3 A(c1, c2, c3);
            auto M_ = glm::determinant(A);
            glm::mat3 BETA(c4, c2, c3);
            auto beta_ = glm::determinant(BETA) / M_;
            glm::mat3 GAMMA(c1, c4, c3);
            auto gamma_ = glm::determinant(GAMMA) / M_;
            glm::mat3 T(c1, c2, c4);
            auto t_ = glm::determinant(T) / M_;

            auto t_test = t_ < t0 || t_ > t1;
            auto gamma_test = gamma_ < 0 || gamma_ > 1;
            auto beta_test = beta_ < 0 || beta_ > (1 - gamma_);

            M = (a * ei_hf) + (b * gf_di) + (c * dh_eg);
            //assert(glm::epsilonEqual(M, M_, glm::epsilon<float>()));

            t = ((f * ak_jb) + (e * jc_al) + (d * bl_kc)) / -M;
            //assert(glm::epsilonEqual(t, t_, glm::epsilon<float>()));
            // test t
            if (t < t0 || t > t1) continue;

            gamma = ((i * ak_jb) + (h * jc_al) + (g * bl_kc)) / M;
            //assert(glm::epsilonEqual(gamma, gamma_, glm::epsilon<float>()));
            // test gamma
            if (gamma < 0 || gamma > 1) continue;

            beta = ((j * ei_hf) + (k * gf_di) + (l * dh_eg)) / M;
            //assert(glm::epsilonEqual(beta, beta_, glm::epsilon<float>()));
            // test beta
            if (beta < 0 || beta > (1 - gamma)) continue;

            // test face
            if (!faceFound) {
                faceFound = true;
                face = {};
                face[0] = vertices_[idx0];
                face[1] = vertices_[idx1];
                face[2] = vertices_[idx2];
            } else {
                Face tmp = {};
                tmp[0] = vertices_[idx0];
                tmp[1] = vertices_[idx1];
                tmp[2] = vertices_[idx2];

                if (face.compareCentroids(localRay[0], tmp)) {
                    face = tmp;
                }
            }
        }

        return faceFound;
    }

   protected:
    std::vector<Vertex::Color> vertices_;
};

// **********************
// LineMesh
// **********************

class LineMesh : public ColorMesh {
   public:
    LineMesh(MeshCreateInfo* pCreateInfo);
};

// **********************
// TextureMesh
// **********************

class TextureMesh : public Mesh {
   public:
    TextureMesh(MeshCreateInfo* pCreateInfo);

    // INIT
    void setSceneData(const Shell::Context& ctx, size_t offset);

    void prepare(const VkDevice& dev, std::unique_ptr<DescriptorResources>& pRes) override;
    void drawSecondary(const VkCommandBuffer& cmd, const VkPipelineLayout& layout, const VkPipeline& pipeline,
                       const VkDescriptorSet& descSet, const std::array<uint32_t, 1>& dynUboOffsets, size_t frameIndex,
                       const VkCommandBufferInheritanceInfo& inheritanceInfo, const VkViewport& viewport,
                       const VkRect2D& scissor) const override;

    // VERTEX
    inline Vertex::Complete getVertexComplete(size_t index) override {
        assert(index < vertices_.size());
        return {vertices_.at(index)};
    }
    inline void addVertex(const Vertex::Complete& v, int32_t index = -1) override {
        if (index == -1) {
            vertices_.push_back(v.getTextureVertex());
        } else {
            assert(index < vertices_.size());
            vertices_[index] = v.getTextureVertex();
        }
        updateBoundingBox(vertices_.back());
    }
    inline virtual const void* getVertexData() const override { return vertices_.data(); }
    inline uint32_t getVertexCount() const { return vertices_.size(); }
    inline VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize p_bufferSize = sizeof(Vertex::Texture) * vertices_.size();
        return p_bufferSize;
    }

    // TEXTURE
    inline uint32_t getTextureOffset() const { return pMaterial_->getTexture().offset; }
    void tryCreateDescriptorSets(std::unique_ptr<DescriptorResources>& pRes);

    void destroy(const VkDevice& dev) override;

   protected:
    std::vector<Vertex::Texture> vertices_;

   private:
    std::vector<VkDescriptorSet> descSets_;
    std::vector<VkCommandBuffer> secCmds_;
};

#endif  // !MESH_H
