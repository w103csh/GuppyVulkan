/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <vector>

#include "Vertex.h"

// COLOR

void Vertex::Color::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(Vertex::Color);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // position
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 0;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Color, position);

    // normal
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 1;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Color, normal);

    // color
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 2;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Color, color);
}

// TEXTURE

void Vertex::Texture::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(Vertex::Texture);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // position
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 0;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, position);

    // normal
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 1;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, normal);

    // texture coordinate
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 2;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, texCoord);

    // image space tangent
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 3;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, tangent);

    // image space bitangent
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 4;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, bitangent);
}

// SCREEN_QUAD

void Vertex::Texture::getScreenQuadInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(Vertex::Texture);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // position
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 0;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, position);

    // texture coordinate
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = 1;
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    createInfoRes.attrDescs.back().offset = offsetof(Vertex::Texture, texCoord);
}
