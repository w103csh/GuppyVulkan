/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * 
 *  This is really like a PCH file. I should make one.
 *
 *  Non-constants files should include this, and not constants files. Constants
 *  files should never include this.
 */

#ifndef CONSTANTS_ALL_H
#define CONSTANTS_ALL_H

#include "Constants.h"
#include "Enum.h"
#include "Types.h"

#include "DescriptorConstants.h"
#include "MeshConstants.h"
#include "ModelConstants.h"
#include "ParticleConstants.h"
#include "PipelineConstants.h"
#include "RenderPassConstants.h"
#include "SamplerConstants.h"
#include "ShaderConstants.h"
#include "StorageConstants.h"
#include "TextureConstants.h"
#include "UniformConstants.h"

#endif  // !CONSTANTS_ALL_H
