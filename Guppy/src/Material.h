
#ifndef MATERIAL_H
#define MATERIAL_H

#include <functional>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include "BufferManager.h"
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
    // THROUGH 0x00000080
    HIDE = 0x00000100,
    // THROUGH 0x00000800
    METAL = 0x00001000,
    // THROUGH 0x00008000
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAG;

struct Info {
    MATERIAL type = MATERIAL::DEFAULT;
    std::shared_ptr<Texture::DATA> pTexture = nullptr;
    // COMMON
    FlagBits flags = PER_MATERIAL_COLOR;
    glm::vec3 diffuseCoeff{1.0f};
    // DEFAULT
    glm::vec3 ambientCoeff{0.1f};
    glm::vec3 specularCoeff{0.9f};
    float opacity = 1.0f;
    float repeat = 1.0f;
    float shininess = SHININESS::MILDLY_SHINY;
    // PBR
    float roughness = 0.0f;
    bool metal = false;
};

static void copyTinyobjData(const tinyobj::material_t& m, Material::Info& materialInfo) {
    materialInfo.shininess = m.shininess;
    materialInfo.ambientCoeff = {m.ambient[0], m.ambient[1], m.ambient[2]};
    materialInfo.diffuseCoeff = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    materialInfo.specularCoeff = {m.specular[0], m.specular[1], m.specular[2]};
}

class Base : public Buffer::Item {
   public:
    Base(Material::Info* pInfo);

    MATERIAL TYPE;

    STATUS getStatus() const {
        if (hasTexture() && pTexture_->status != STATUS::READY) {
            return STATUS::PENDING_TEXTURE;
        }
        return STATUS::READY;
    }

    inline bool hasTexture() const { return pTexture_ != nullptr; }
    inline const std::shared_ptr<Texture::DATA> getTexture() const { return pTexture_; }

   private:
    std::shared_ptr<Texture::DATA> pTexture_;
};

namespace Default {

struct DATA {
    glm::vec3 Ka;
    FlagBits flags;
    // 16
    glm::vec3 Kd;
    float opacity;
    // 16
    glm::vec3 Ks;
    float shininess;
    // 16
    float xRepeat;
    float yRepeat;
    // 8 (8 rem)
};

static DATA getData(const Material::Info* pInfo) {
    float xRepeat, yRepeat;
    xRepeat = yRepeat = pInfo->repeat;

    // Deal with non-square images
    if (pInfo->pTexture != nullptr) {
        auto aspect = pInfo->pTexture->aspect;
        if (aspect > 1.0f) {
            yRepeat *= aspect;
        } else if (aspect < 1.0f) {
            xRepeat *= (1 / aspect);
        }
    }

    return {pInfo->ambientCoeff,
            pInfo->flags,
            pInfo->diffuseCoeff,
            pInfo->opacity,
            pInfo->specularCoeff,
            pInfo->shininess,
            xRepeat,
            yRepeat};
}

}  // namespace Default

// class Base {
//   public:
//    struct DATA {
//        glm::vec3 Ka;
//        FlagBits flags;
//        // 16
//        glm::vec3 Kd;
//        float opacity;
//        // 16
//        glm::vec3 Ks;
//        float shininess;
//        // 16
//        float xRepeat;
//        float yRepeat;
//        // 8 (8 rem)
//    };
//
//    static Material::Default::DATA getData(const std::unique_ptr<Material::Base>& pMaterial);
//
//    Base(Material::Info* pCreateInfo)
//        : flags_(pCreateInfo->flags),
//          ambientCoeff_(pCreateInfo->ambientCoeff),
//          diffuseCoeff_(pCreateInfo->diffuseCoeff),
//          specularCoeff_(pCreateInfo->specularCoeff),
//          opacity_(pCreateInfo->opacity),
//          repeat_(pCreateInfo->repeat),
//          // xRepeat_(pCreateInfo->xRepeat),
//          // yRepeat_(pCreateInfo->yRepeat),
//          shininess_(pCreateInfo->shininess),
//          pTexture_(pCreateInfo->pTexture),
//          // PBR
//          roughness_(pCreateInfo->roughness)
//    //
//    {}
//
//    STATUS getStatus() const;
//
//    inline FlagBits getFlags() const { return flags_; }
//    inline glm::vec3 getAmbientCoeff() const { return ambientCoeff_; }
//    inline glm::vec3 getDiffuseCoeff() const { return diffuseCoeff_; }
//    inline glm::vec3 getSpecularCoeff() const { return specularCoeff_; }
//    inline float getOpacity() const { return opacity_; }
//    inline float getRepeat() const { return repeat_; }
//    inline float getShininess() const { return shininess_; }
//    inline const std::shared_ptr<Texture::Data>& getTexture() const { return pTexture_; }
//    inline float getRoughness() const { return roughness_; }
//
//    inline void setFlags(FlagBits flags) { flags_ = flags; }
//    inline void setColor(glm::vec3 c) { ambientCoeff_ = diffuseCoeff_ = c; }
//    inline void setAmbientColor(glm::vec3 c) { ambientCoeff_ = c; }
//    inline void setDiffuseColor(glm::vec3 c) { diffuseCoeff_ = c; }
//    inline void setSpecularColor(glm::vec3 c) { specularCoeff_ = c; }
//    inline void setOpacity(float o) { opacity_ = o; }
//    inline void setRepeat(float r) { repeat_ = r; }
//    // inline void setXRepeat(float r) { xRepeat_ = r; }
//    // inline void setYRepeat(float r) { yRepeat_ = r; }
//    inline void setShininess(float s) { shininess_ = s; }
//    inline void setRoughness(float r) { roughness_ = r; }
//
//    inline void setTexture(std::shared_ptr<Texture::Data> pTexture) {
//        flags_ = flags_ | (BITS_MAX_ENUM & PER_TEXTURE_COLOR);
//        pTexture_ = pTexture;
//    }
//    inline bool hasTexture() const { return pTexture_.get() != nullptr; }
//
//   private:
//    // DEFAULT
//    FlagBits flags_;
//    glm::vec3 ambientCoeff_, diffuseCoeff_, specularCoeff_;
//    float opacity_, repeat_;  //, xRepeat_, yRepeat_;
//    float shininess_;         // phong exponent
//    std::shared_ptr<Texture::Data> pTexture_;
//    // PBR
//    float roughness_;
//};

};  // namespace Material

#endif  // !MATERIAL_H