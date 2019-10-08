
#include "SoundHandler.h"

#include <assert.h>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <sstream>

namespace {
void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line) {
    if (result != FMOD_OK) {
        std::stringstream ss;
        ss << file << "(" << line << "): FMOD error " << result << " - " << FMOD_ErrorString(result);
        assert(false && ss.str().c_str());
    }
}
#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILE__, __LINE__)
}  // namespace

Sound::Handler::Handler(Shell *pShell) : Shell::Handler(pShell) {}

void Sound::Handler::init() {
    // FMOD::System *system;
    //// FMOD::Sound *sound, *sound_to_play;
    // FMOD::Channel *channel = 0;
    // FMOD_RESULT result;
    // unsigned int version;
    // void *extradriverdata = 0;
    // int numsubsounds;

    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);
    if (result != FMOD_OK) result = system->getVersion(&version);
    ERRCHECK(result);
}

void Sound::Handler::reset() {
    /*
        Shut down
    */
    // result = sound->release(); /* Release the parent, not the sound that was retrieved with getSubSound. */
    // ERRCHECK(result);
    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);
}