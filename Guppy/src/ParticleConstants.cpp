
#include "ParticleConstants.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Descriptor.h"
#include "Storage.h"

void Storage::Vector4::Particle::Attractor::SetDataPosition(Descriptor::Base* pItem, Vector4::DATA*& pData,
                                                            const Storage::Vector4::CreateInfo* pCreateInfo) {
    assert(pItem->BUFFER_INFO.count == pCreateInfo->localSize.x * pCreateInfo->localSize.y * pCreateInfo->localSize.z);
    // Initial positions of the particles
    glm::vec4 p{0.0f, 0.0f, 0.0f, 1.0f};
    float dx = 2.0f / (pCreateInfo->localSize.x - 1), dy = 2.0f / (pCreateInfo->localSize.y - 1),
          dz = 2.0f / (pCreateInfo->localSize.z - 1);
    // We want to center the particles at (0,0,0)
    glm::mat4 transf = glm::translate(glm::mat4(1.0f), glm::vec3(-1, -1, -1));
    uint32_t idx = 0;
    for (uint32_t i = 0; i < pCreateInfo->localSize.x; i++) {
        for (uint32_t j = 0; j < pCreateInfo->localSize.y; j++) {
            for (uint32_t k = 0; k < pCreateInfo->localSize.z; k++) {
                pData[idx].x = dx * i;
                pData[idx].y = dy * j;
                pData[idx].z = dz * k;
                pData[idx].w = 1.0f;
                pData[idx] = transf * pData[idx];
                idx++;
            }
        }
    }
}
