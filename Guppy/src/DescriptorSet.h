/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <array>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ConstantsAll.h"
#include "DescriptorConstants.h"
#include "Handlee.h"

namespace Descriptor {

class Handler;

namespace Set {

const uint32_t OFFSET_ALL = UINT32_MAX;

using mapKey = std::array<uint32_t, 4>;

class Base : public Handlee<Descriptor::Handler> {
    friend class Descriptor::Handler;

   public:
    const DESCRIPTOR_SET TYPE;
    const std::string MACRO_NAME;

    inline bool isInitialized() const { return !resources_[defaultResourceOffset_].pipelineTypes.empty(); }

    constexpr const auto& getBindingMap() const { return bindingMap_; }
    inline const auto& getResource(const uint32_t& offset) const { return resources_[offset]; }
    const Descriptor::OffsetsMap getDescriptorOffsets(const uint32_t& offset) const;
    constexpr const auto& getDefaultResourceOffset() const { return defaultResourceOffset_; }
    constexpr const auto& getSetCount() const { return setCount_; }

    void update(const uint32_t imageCount);
    void updateOffsets(const Uniform::offsetsMap offsetsMap, const DESCRIPTOR& descType, const PIPELINE& pipelineType);

    bool hasTextureMaterial() const;
    bool hasStorageBufferDynamic() const;

    // ITERATOR
    void findResourceForPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type);
    void findResourceForDefaultPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type);
    void findResourceSimilar(std::vector<Resource>::iterator& it, const PIPELINE& piplineType,
                             const std::set<PASS>& passTypes, const DESCRIPTOR& descType, const Uniform::offsets& offsets);

   protected:
    Base(Handler& handler, const CreateInfo* pCreateInfo);

   private:
    inline auto& getDefaultResource() { return resources_[defaultResourceOffset_]; }

    Descriptor::bindingMap bindingMap_;
    uint8_t setCount_;
    const uint32_t defaultResourceOffset_;
    std::vector<Resource> resources_;
    Uniform::offsetsMap uniformOffsets_;

    /**
     * Key:
     *  Dynamic uniform/storage buffer offsets
     * Value:
     *  Resource offset map
     */
    std::map<mapKey, std::map<uint32_t, resourceInfoMapSetsPair>> descriptorSetsMap_;
};

}  // namespace Set
}  // namespace Descriptor

#endif  // !DESCRIPTOR_SET_H