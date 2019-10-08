
#ifndef SOUND_HANDLER_H
#define SOUND_HANDLER_H

#include "Shell.h"

// REMOVE THE FMOD STUFF!!!!
#include <fmod.hpp>

namespace Sound {

class Handler : public Shell::Handler {
   public:
    Handler(Shell *pShell);

    void init();
    void reset() override;

   private:
    FMOD::System *system;
    // FMOD::Sound *sound, *sound_to_play;
    FMOD::Channel *channel = 0;
    FMOD_RESULT result;
    unsigned int version;
    void *extradriverdata = 0;
};

}  // namespace Sound

#endif  // !SOUND_HANDLER_H