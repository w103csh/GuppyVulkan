/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifdef USE_FMOD

#include "SoundFModHandler.h"

#include <assert.h>
#include <fmod_errors.h>
#include <sstream>
#include <string>

#include "ConstantsAll.h"

// HELPERS

namespace {

const std::string MEDIA_PATH = ROOT_PATH + "media/";
const std::string MP3_OCEAN_WAVES = MEDIA_PATH + "Ocean_Waves-Mike_Koenig-980635527.mp3";

constexpr const char *getSound(const Sound::TYPE type) {
    switch (type) {
        case Sound::TYPE::OCEAN_WAVES:
            return MP3_OCEAN_WAVES.c_str();
        default:
            assert(false && "Unhandled sound");
            exit(EXIT_FAILURE);
    }
}

}  // namespace

// HANDLER

Sound::FModHandler::FModHandler(Shell *pShell) : Handler(pShell), system_(nullptr), samplerate_(48000) {}

void Sound::FModHandler::init() {
    FMOD_RESULT result;
    unsigned int version;
    void *extradriverdata = 0;

    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system_);
    if (result != FMOD_OK) FMOD_ERRCHECK(system_->getVersion(&version));

    FMOD_ERRCHECK(system_->getVersion(&version));

    if (version < FMOD_VERSION) {
        assert(false && "FMOD lib version doesn't match header version");
    }

    FMOD_ERRCHECK(system_->init(32, FMOD_INIT_NORMAL, extradriverdata));
    assert(system_ != nullptr);

    FMOD_ERRCHECK(system_->getSoftwareFormat(const_cast<int *>(&samplerate_), nullptr, nullptr));
}

bool Sound::FModHandler::start(const TYPE type, const StartInfo *pStartInfo, const EffectInfo *pEffectInfo) {
    if (pStartInfo == nullptr) {
        assert(false);
        return false;
    }

    auto it = soundChannelMap_.find(type);
    if (it != soundChannelMap_.end()) {
        return false;
    } else {
        auto insertPair = soundChannelMap_.insert({type, {nullptr, nullptr}});
        assert(insertPair.second);
        it = insertPair.first;
    }
    auto &[sound, channel] = it->second;

    FMOD_ERRCHECK(system_->createStream(getSound(type), FMOD_LOOP_NORMAL | FMOD_2D, 0, &sound));

    int numsubsounds;
    FMOD_ERRCHECK(sound->getNumSubSounds(&numsubsounds));
    // There is something about releasing only the parents of sounds in the example so just fail until you understand
    // how to clean it up properly
    assert(numsubsounds == 0);

    // Settings
    bool setStartVolume = pStartInfo->volume != BAD_VOLUME;
    bool startPaused = setStartVolume;
    if (pEffectInfo != nullptr) {
        switch (pEffectInfo->effect) {
            case EFFECT::FADE: {
                startPaused = true;
            } break;
            default:;
        }
    }

    // Play the sound.
    FMOD_ERRCHECK(system_->playSound(sound, 0, startPaused, &channel));

    // Start info settings after initial pause.
    if (startPaused) {
        if (setStartVolume) FMOD_ERRCHECK(channel->setVolume(pStartInfo->volume));
    }

    // EFFECT
    if (pEffectInfo != nullptr) effect(channel, *pEffectInfo);

    // Unpause before returning
    if (startPaused) FMOD_ERRCHECK(channel->setPaused(false));
    return true;
}

void Sound::FModHandler::update() {
    FMOD_ERRCHECK(system_->update());

    FMOD_RESULT result;
    unsigned int ms = 0;
    unsigned int lenms = 0;
    bool playing = false;
    bool paused = false;

    // This is all kind of backwards so that the tests from the example are run. Really the only thing you probably
    // need to check every frame is effects????
    bool erase;
    for (auto itChannel = soundChannelMap_.begin(); itChannel != soundChannelMap_.end();) {
        auto &[sound, channel] = itChannel->second;
        erase = false;

        // EFFECTS
        auto range = effectMap_.equal_range(itChannel->first);
        for (auto itEffect = range.first; itEffect != range.second; itEffect = effectMap_.erase(itEffect)) {
            erase |= effect(channel, itEffect->second);
        }

        // Below are tests from an example.
        if (channel && !erase) {
            result = channel->isPlaying(&playing);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE)) {
                FMOD_ERRCHECK(result);
            }

            result = channel->getPaused(&paused);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE)) {
                FMOD_ERRCHECK(result);
            }

            result = channel->getPosition(&ms, FMOD_TIMEUNIT_MS);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE)) {
                FMOD_ERRCHECK(result);
            }

            result = sound->getLength(&lenms, FMOD_TIMEUNIT_MS);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE)) {
                FMOD_ERRCHECK(result);
            }
        }

        if (erase)
            itChannel = soundChannelMap_.erase(itChannel);
        else
            ++itChannel;
    }
}

void Sound::FModHandler::pause(const TYPE type) {
    auto it = soundChannelMap_.find(type);
    if (it != soundChannelMap_.end()) {
        bool paused;
        FMOD_ERRCHECK(it->second.second->getPaused(&paused));
        FMOD_ERRCHECK(it->second.second->setPaused(!paused));
    }
}

void Sound::FModHandler::stop(const TYPE type) {
    auto it = soundChannelMap_.find(type);
    if (it != soundChannelMap_.end()) {
        /* Release the parent, not the sound that was retrieved with getSubSound. */
        FMOD_ERRCHECK(it->second.first->release());
        soundChannelMap_.erase(it);
    }
}

void Sound::FModHandler::destroy() {
    for (auto &[type, pair] : soundChannelMap_) {
        // If there is a parent sound then this is not valid
        FMOD_ERRCHECK(pair.first->release());
    }
    FMOD_ERRCHECK(system_->close());
    FMOD_ERRCHECK(system_->release());
}

void Sound::FModHandler::errorCheck(FMOD_RESULT result, const char *file, int line) {
    if (result != FMOD_OK) {
        std::stringstream ss;
        ss << file << "(" << line << "): FMOD error " << result << " - " << FMOD_ErrorString(result);
        shell().log(Shell::LogPriority::LOG_ERR, ss.str().c_str());
        assert(false);
    }
}

bool Sound::FModHandler::effect(FMOD::ChannelControl *channel, const EffectInfo &info) {
    switch (info.effect) {
        case EFFECT::FADE: {
            float volume;
            FMOD_ERRCHECK(channel->getVolume(&volume));
            unsigned long long dspclock;
            FMOD_ERRCHECK(channel->getDSPClock(&dspclock, nullptr));
            // Sometimes this delay seems necessary... I don't know.
            // dspclock += 1024 * 1;
            // FMOD_ERRCHECK(channel->setDelay(dspclock, 0));
            FMOD_ERRCHECK(channel->addFadePoint(dspclock, volume));
            auto futureclock = getDSP(dspclock, info.time);
            FMOD_ERRCHECK(channel->addFadePoint(futureclock, info.volume));

            // FADE OUT DID NOT WORK. THE VOLUME WAS NOT SET PROPERLY AFTER A "FADE IN"

            // if (info.playback == PLAYBACK::STOP) {
            //    FMOD_ERRCHECK(channel->setDelay(0, futureclock, true));
            //    return true;
            //} else if (info.playback == PLAYBACK::PAUSE) {
            //    FMOD_ERRCHECK(channel->setDelay(0, futureclock, false));
            //}
        } break;
        case EFFECT::DO_NOTHING:
        default: {
            assert(false && "Unhandled effect type");
            exit(EXIT_FAILURE);
        }
    }
    return true;
}

#endif  // !USE_FMOD