
#ifndef MATERIAL_H
#define MATERIAL_H

#include <functional>
#include <glm/glm.hpp>

#include "Constants.h"
#include "BufferItem.h"
#include "Descriptor.h"
#include "Texture.h"

namespace Material {

enum SHININESS {
    EGGSHELL = 10,
    MILDLY_SHINY = 100,
    GLOSSY = 1000,
    MIRROR_LIKE = 10000,
};

typedef enum FLAG {
    PER_MATERIAL_COLOR = 0x00000001,
    PER_VERTEX_COLOR = 0x00000002,
    PER_TEXTURE_COLOR = 0x00000004,
    // THROUGH 0x00000008
    MODE_LAMERTIAN = 0x00000010,
    MODE_BLINN_PHONG = 0x00000020,
    MODE_TOON_SHADE = 0x00000040,
    // THROUGH 0x00000080
    HIDE = 0x00000100,
    // THROUGH 0x00000800
    METAL = 0x00001000,
    // THROUGH 0x00008000
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAG;

struct CreateInfo : public Buffer::CreateInfo {
    std::shared_ptr<Texture::Base> pTexture = nullptr;
    // COMMON
    glm::vec3 color = {0.3f, 0.3f, 0.3f};
    FlagBits flags = Material::FLAG::PER_MATERIAL_COLOR;
    float opacity = 1.0f;
    float repeat = 1.0f;
};

struct DATA {
    glm::vec3 color;  // diffuse coefficient
    float opacity;
    // 16
    FlagBits flags;
    FlagBits texFlags;
    float xRepeat;
    float yRepeat;
    // 16
};

// **********************
//      Base
// **********************

class Base : public virtual Buffer::Item, public Descriptor::Interface {
   public:
    const MATERIAL TYPE;

    const STATUS& getStatus() const { return status_; }

    inline void updateStatus() {
        assert(status_ != STATUS::READY && "This shouldn't be used to check status");
        if (hasTexture() && pTexture_->status == STATUS::READY) {
            setTextureData();
            DIRTY = true;
            status_ = STATUS::READY;
        }
    }

    inline bool hasTexture() const { return pTexture_ != nullptr; }
    inline const std::shared_ptr<Texture::Base> getTexture() const { return pTexture_; }

    virtual FlagBits getFlags() = 0;
    virtual void setFlags(FlagBits flags) = 0;
    virtual void setTextureData(){};

    // OBJ
    virtual void setTinyobjData(const tinyobj::material_t& m){};

    // DESCRIPTOR
    inline void setWriteInfo(VkWriteDescriptorSet& write) const override { write.pBufferInfo = &BUFFER_INFO.bufferInfo; }

   protected:
    Base(const MATERIAL&& type, Material::CreateInfo* pCreateInfo);

    STATUS status_;

    // TEXTURE
    std::shared_ptr<Texture::Base> pTexture_;
    float repeat_;
};

// **********************
//      Default
// **********************

namespace Default {

struct DATA : public Material::DATA {
    glm::vec3 Ka{0.1f};
    float shininess = SHININESS::MILDLY_SHINY;
    // 16
    alignas(16) glm::vec3 Ks{0.9f};
    // 16 (4 rem)
};

struct CreateInfo : public Material::CreateInfo {
    glm::vec3 ambientCoeff{0.1f};
    glm::vec3 specularCoeff{0.9f};
    float shininess = SHININESS::MILDLY_SHINY;
};

class Base : public Buffer::DataItem<DATA>, public Material::Base {
   public:
    Base(const Buffer::Info&& info, Default::DATA* pData, Default::CreateInfo* pCreateInfo);

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override;
    void setTinyobjData(const tinyobj::material_t& m) override;

   private:
    void setData() override { DIRTY = true; };
};

static void copyTinyobjData(const tinyobj::material_t& m, Default::CreateInfo& materialInfo) {
    materialInfo.shininess = m.shininess;
    materialInfo.ambientCoeff = {m.ambient[0], m.ambient[1], m.ambient[2]};
    materialInfo.color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    materialInfo.specularCoeff = {m.specular[0], m.specular[1], m.specular[2]};
}

}  // namespace Default

};  // namespace Material

#endif  // !MATERIAL_H