/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SOUND_HANDLER_H
#define SOUND_HANDLER_H

#include <assert.h>
#include <float.h>
#include <map>

#include "Shell.h"

/* TODO: For handlers like this one that are dependent on something optional I believe
 *  there is a way to split the different implementations across multiple files, and then
 *  #ifdef out the ones that aren't being. Then there would be no time wasted on virtual
 *  function lookup.
 */
namespace Sound {

constexpr float BAD_VOLUME = -FLT_MAX;
constexpr float START_ZERO_VOLUME = 0.01f;

enum class TYPE {
    OCEAN_WAVES,
};

enum class EFFECT {
    DO_NOTHING,
    FADE,
};

enum class PLAYBACK {
    DO_NOTHING,
    START,
    STOP,
    PAUSE,
};

struct EffectInfo {
    EFFECT effect = EFFECT::DO_NOTHING;
    float time = 0.0f;
    float volume = 1.0f;
    PLAYBACK playback = PLAYBACK::DO_NOTHING;
    bool togglePaused = false;
};

struct StartInfo {
    float volume = BAD_VOLUME;
};

class Handler : public Shell::Handler {
   public:
    Handler(Shell *pShell);

    virtual void init() override {}
    virtual void update(const double) override {}
    virtual bool start(const TYPE type, const StartInfo *pStartInfo, const EffectInfo *pEffectInfo = nullptr) {
        return false;
    }
    virtual void pause(const TYPE type) {}
    virtual void stop(const TYPE type) {}
    virtual void destroy() override {}

    // This probably shouldn't just blindly add something to map that is going to be checked every cycle
    inline void addEffect(const TYPE type, const EffectInfo *pInfo) {
        if (pInfo != nullptr)
            effectMap_.insert({type, *pInfo});
        else
            assert(false);
    }

   protected:
    std::multimap<TYPE, EffectInfo> effectMap_;
};

}  // namespace Sound

#endif  // !SOUND_HANDLER_H