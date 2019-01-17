/*
    Physically-based Rendering Shader

    This comes from the cookbook, and is a physically-based reflection
    model. I will try to fill this out more once I can do more work on
    the math.
*/

#ifndef SHADER_PBR_H
#define SHADER_PBR_H

#include "Shader.h"

namespace Shader {

class PBRColor : public Base {
   public:
    PBRColor(const VkDevice &dev);
};

}  // namespace Shader

#endif  // !SHADER_PBR_H