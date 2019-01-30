
#include "UniformHandler.h"

#include "PBR.h"
#include "Shell.h"

Uniform::Handler::Handler(Game *pGame) : Game::Handler(pGame) {}
// UNIFORM BUFFERS
// uniforms_{
//    // DEFAULT
//    std::make_unique<Uniform::Default::CameraPerspective>(),
//    std::make_unique<Uniform::Default::LightPositional>(),
//    std::make_unique<Uniform::Default::LightSpot>(),
//    std::make_unique<Uniform::Default::Material>(),
//    // PBR
//    std::make_unique<Uniform::PBR::LightPositional>(),
//    std::make_unique<Uniform::PBR::Material>(),
//} {}