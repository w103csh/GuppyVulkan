/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include <functional>
#include <glm/glm.hpp>

#include "ConstantsAll.h"
#include "BufferItem.h"
#include "Descriptor.h"
#include "Obj3d.h"
#include "Texture.h"

namespace Material {

// clang-format off
using FLAG = enum : FlagBits {
    PER_MATERIAL_COLOR =        0x00000001,
    PER_VERTEX_COLOR =          0x00000002,
    PER_TEXTURE_COLOR =         0x00000004,
    PER_MAX_ENUM =              0x0000000F,
    //
    MODE_LAMERTIAN =            0x00000010,
    MODE_BLINN_PHONG =          0x00000020,
    MODE_TOON_SHADE =           0x00000040,
    MODE_FLAT_SHADE =           0x00000080,
    //
    HIDE =                      0x00000100,
    SKYBOX =                    0x00000200,
    REFLECT =                   0x00000400,
    REFRACT =                   0x00000800,
    //
    METAL =                     0x00001000,
    //
    IS_MESH =                   0x00010000,
    //
    BITS_MAX_ENUM =             0x7FFFFFFF
};
// clang-format on

// Values for shininess
constexpr float SHININESS_EGGSHELL = 10.0f;
constexpr float SHININESS_MILDLY_SHINY = 100.0f;
constexpr float SHININESS_GLOSSY = 1000.0f;
constexpr float SHININESS_MIRROR_LIKE = 10000.0f;

// Values for index of refraction
constexpr float REFRACT_ETA_AIR = 1.0003f;
constexpr float REFRACT_ETA_WATER = 1.333f;
constexpr float REFRACT_ETA_CROWN_GLASS = 1.517f;
constexpr float REFRACT_ETA_DENSE_FLINT_GLASS = 1.655f;
constexpr float REFRACT_ETA_DIAMOND = 2.417f;

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

// BASE

class Base : public Descriptor::Base {
   public:
    virtual_inline STATUS getStatus() const { return status_; }
    inline bool hasTexture() const { return pTexture_ != nullptr; }
    virtual_inline const auto& getTexture() const { return pTexture_; }
    virtual_inline const auto& getRepeat() const { return repeat_; }

    virtual FlagBits getFlags() = 0;
    virtual void setFlags(FlagBits flags) = 0;
    // TODO: this isn't actually enforced anywhere. There should be an pure virtual function on Descriptor::Base
    // for getData() or something, and then this could be enforced in a setData function.
    virtual void setTextureData() {}

    // OBJ
    virtual void setTinyobjData(const tinyobj::material_t& m) {}

   protected:
    Base(const DESCRIPTOR&& descType, const CreateInfo* pCreateInfo);

    STATUS status_;

    // TEXTURE
    std::shared_ptr<Texture::Base> pTexture_;
    float repeat_;
};

// FUNCTIONS

STATUS SetDefaultTextureData(Material::Base* pMaterial, DATA* pData);

// DEFAULT

namespace Default {

struct DATA : public Material::DATA {
    glm::vec3 Ka{0.1f};
    float shininess = SHININESS_MILDLY_SHINY;
    // 16
    glm::vec3 Ks{0.9f};
    float eta = 0.94f;
    // 16
    alignas(16) float reflectionFactor = 0.85f;
};

struct CreateInfo : public Material::CreateInfo {
    glm::vec3 ambientCoeff{0.1f};
    glm::vec3 specularCoeff{0.9f};
    float shininess = SHININESS_MILDLY_SHINY;
    float eta = REFRACT_ETA_CROWN_GLASS;
    float reflectionFactor = 0.85f;
};

constexpr void SetData(const CreateInfo* pCreateInfo, DATA* pData) {
    pData->color = pCreateInfo->color;
    pData->flags = pCreateInfo->flags;
    pData->Ka = pCreateInfo->ambientCoeff;
    pData->Ks = pCreateInfo->specularCoeff;
    pData->opacity = pCreateInfo->opacity;
    pData->shininess = pCreateInfo->shininess;
    pData->eta = pCreateInfo->eta;
    pData->reflectionFactor = pCreateInfo->reflectionFactor;
}

constexpr void SetTinyobjData(const tinyobj::material_t& m, DATA* pData) {
    pData->shininess = m.shininess;
    pData->Ka = {m.ambient[0], m.ambient[1], m.ambient[2]};
    pData->color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    pData->Ks = {m.specular[0], m.specular[1], m.specular[2]};
}

class Base : public Material::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, Default::DATA* pData, const Default::CreateInfo* pCreateInfo);

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override { status_ = SetDefaultTextureData(this, pData_); }
    void setTinyobjData(const tinyobj::material_t& m) override;

   private:
    void setData(const uint32_t index = 0) override { dirty = true; };
};

}  // namespace Default

// OBJ3D

namespace Obj3d {

struct DATA : public Material::Default::DATA {
    glm::mat4 model;
};

struct CreateInfo : public Material::Default::CreateInfo {
    glm::mat4 model{1.0f};
};

class Default : public ::Obj3d::AbstractBase, public Material::Base, public Buffer::DataItem<DATA> {
   public:
    Default(const Buffer::Info&& info, Default::DATA* pData, const CreateInfo* pCreateInfo)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),
          Base(UNIFORM_DYNAMIC::MATERIAL_OBJ3D, pCreateInfo),
          Buffer::DataItem<Default::DATA>(pData) {
        SetData(pCreateInfo, pData_);
        setTextureData();
        pData_->model = pCreateInfo->model;
        setData();
    }

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override { status_ = SetDefaultTextureData(this, pData_); }
    void setTinyobjData(const tinyobj::material_t& m) override {
        SetTinyobjData(m, pData_);
        setData();
    }

    const glm::mat4& model(const uint32_t index = 0) const override { return pData_->model; }
};

}  // namespace Obj3d

};  // namespace Material

#endif  // !MATERIAL_H