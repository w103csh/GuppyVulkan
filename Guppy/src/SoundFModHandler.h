
#ifdef USE_FMOD
#ifndef SOUND_FMOD_HANDLER_H
#define SOUND_FMOD_HANDLER_H

#include <fmod.hpp>
#include <map>

#include "Constants.h"
#include "SoundHandler.h"

namespace Sound {

class FModHandler : public Handler {
   private:
    void errorCheck(FMOD_RESULT result, const char *file, int line);
#define FMOD_ERRCHECK(_result) errorCheck(_result, __FILE__, __LINE__)

   public:
    FModHandler(Shell *pShell);

    void init() override;
    bool start(const TYPE type, const StartInfo *pStartInfo, const EffectInfo *pEffectInfo = nullptr) override;
    void update(const double) override;
    void pause(const TYPE type) override;
    void stop(const TYPE type) override;
    void destroy() override;

   private:
    virtual_inline unsigned long long getDSP(unsigned long long nCurrentClock, const double t) {
        return nCurrentClock + static_cast<long long>(t * static_cast<long long>(samplerate_));
    }

    bool effect(FMOD::ChannelControl *channel, const EffectInfo &info);

    FMOD::System *system_;
    const int samplerate_;
    std::map<TYPE, std::pair<FMOD::Sound *, FMOD::Channel *>> soundChannelMap_;
};

}  // namespace Sound

#endif  // !SOUND_FMOD_HANDLER_H
#endif  // !USE_FMOD