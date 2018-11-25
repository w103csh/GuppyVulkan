
#ifndef MATERIAL_H
#define MATERIAL_H

#include <functional>
#include <glm\glm.hpp>

#include "Texture.h"

class Material {
   public:
    typedef enum FLAGS {
        PER_MATERIAL_COLOR = 0x00000001,
        PER_VERTEX_COLOR = 0x00000002,
        PER_TEXTURE_COLOR = 0x00000004,
        // through 0x00000008
        MODE_LAMERTIAN = 0x00000010,
        MODE_BLINN_PHONG = 0x00000020,
        // THROUGH 0x00000080
        BITS_MAX_ENUM = 0x7FFFFFFF
    } FLAGS;

    enum SHININESS {
        EGGSHELL = 10,
        MILDLY_SHINY = 100,
        GLOSSY = 1000,
        MIRROR_LIKE = 10000,
    };

    struct Data {
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

    Material(FlagBits flags = PER_MATERIAL_COLOR | MODE_LAMERTIAN)
        : flags_(flags),
          ambientCoeff_(0.1f),
          diffuseCoeff_(1.0f),
          specularCoeff_(0.9f),
          opacity_(1.0f),
          repeat_(1.0f),
          // xRepeat_(1.0f),
          // yRepeat_(1.0f),
          shininess_(MILDLY_SHINY),
          pTexture_(nullptr){};

    Material(std::shared_ptr<Texture::Data> pTexture)
        : flags_(PER_TEXTURE_COLOR | MODE_LAMERTIAN),
          ambientCoeff_(0.1f),
          diffuseCoeff_(1.0f),
          specularCoeff_(0.9f),
          opacity_(1.0f),
          repeat_(1.0f),
          // xRepeat_(1.0f),
          // yRepeat_(1.0f),
          shininess_(MILDLY_SHINY),
          pTexture_(pTexture){};

    STATUS getStatus();
    Data getData() const;
    inline const Texture::Data& getTexture() const { return (*pTexture_); }

    inline void setFlags(FlagBits flags) { flags_ = flags; }
    inline void setColor(glm::vec3 c) { ambientCoeff_ = diffuseCoeff_ = c; }
    inline void setAmbientColor(glm::vec3 c) { ambientCoeff_ = c; }
    inline void setDiffuseColor(glm::vec3 c) { diffuseCoeff_ = c; }
    inline void setSpecularColor(glm::vec3 c) { specularCoeff_ = c; }
    inline void setOpacity(float o) { opacity_ = o; }
    inline void setTexture(std::shared_ptr<Texture::Data> pTexture) {
        flags_ = flags_ | (BITS_MAX_ENUM & PER_TEXTURE_COLOR);
        pTexture_ = pTexture;
    }
    inline void setRepeat(float r) { repeat_ = r; }
    // inline void setXRepeat(float r) { xRepeat_ = r; }
    // inline void setYRepeat(float r) { yRepeat_ = r; }
    inline void setShininess(float s) { shininess_ = s; }

    inline bool hasTexture() const { return pTexture_.get() != nullptr; }

   private:
    FlagBits flags_;
    glm::vec3 ambientCoeff_, diffuseCoeff_, specularCoeff_;
    float opacity_, repeat_;  //, xRepeat_, yRepeat_;
    float shininess_;         // phong exponent
    std::shared_ptr<Texture::Data> pTexture_;
};

#endif  // !MATERIAL_H