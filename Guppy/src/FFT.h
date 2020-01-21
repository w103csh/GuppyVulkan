/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef FFT_H
#define FFT_H

#include <string_view>

#include "Pipeline.h"

// TEXTURE
namespace Texture {
struct CreateInfo;
namespace FFT {
constexpr std::string_view TEST_ID = "FFT Test Texture";
CreateInfo MakeTestTex();
}  // namespace FFT
}  // namespace Texture

// SHADER
namespace Shader {
extern const CreateInfo FFT_ONE_COMP_CREATE_INFO;
}  // namespace Shader

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo FFT_DEFAULT_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace FFT {
class OneComponent : public Compute {
   public:
    OneComponent(Handler& handler);
};
}  // namespace FFT
}  // namespace Pipeline

#endif  // FFT_H