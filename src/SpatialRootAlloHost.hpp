#pragma once

#include "al/io/al_AudioIO.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_ParameterServer.hpp"

#include "EngineSession.hpp"
#include "RealtimeTypes.hpp"

#include <string>
#include <atomic>

struct SpatialRootHostConfig {
    int sampleRate = 48000;
    int blockSize = 512;
    std::string scenePath;
    std::string sourcesFolder;
    std::string admFile;
    std::string layoutPath;
};

class SpatialRootAlloHost {
public:
    bool setup(const SpatialRootHostConfig& config);
    void update(double dt);
    void renderAudio(al::AudioIOData& io);

    void setPaused(bool paused);
    void setMasterGainDb(float dB);
    void setDbapFocus(float focus);
    void setSpeakerMixDb(float dB);
    void setSubMixDb(float dB);
    void setElevationMode(int mode);
    void cycleElevationMode();

    bool isReady() const { return mReady; }
    bool isPaused() const { return mPaused; }
    const std::string& lastError() const { return mLastError; }
    int elevationMode() const { return mElevationMode; }

private:
    EngineSession mSession;
    bool mReady = false;
    bool mPaused = true;
    int mElevationMode = 0;
    std::string mLastError;
};

inline bool SpatialRootAlloHost::setup(const SpatialRootHostConfig& config)
{
    EngineOptions opts;
    opts.sampleRate = config.sampleRate;
    opts.bufferSize = config.blockSize;
    opts.oscPort = 0;
    opts.elevationMode = ElevationMode::RescaleAtmosUp;

    if (!mSession.configureEngine(opts)) {
        mLastError = "configureEngine failed: " + mSession.getLastError();
        return false;
    }

    SceneInput sceneIn;
    sceneIn.scenePath = config.scenePath;
    sceneIn.sourcesFolder = config.sourcesFolder;
    sceneIn.admFile = config.admFile;

    if (!mSession.loadScene(sceneIn)) {
        mLastError = "loadScene failed: " + mSession.getLastError();
        return false;
    }

    LayoutInput layoutIn;
    layoutIn.layoutPath = config.layoutPath;

    if (!mSession.applyLayout(layoutIn)) {
        mLastError = "applyLayout failed: " + mSession.getLastError();
        return false;
    }

    RuntimeParams rParams = RuntimeParams::defaults();
    if (!mSession.configureRuntime(rParams)) {
        mLastError = "configureRuntime failed: " + mSession.getLastError();
        return false;
    }

    if (!mSession.prepareForExternalAudioCallback()) {
        mLastError = "prepareForExternalAudioCallback failed: " + mSession.getLastError();
        return false;
    }

    mReady = true;
    mPaused = true;
    mSession.setPaused(true);
    return true;
}

inline void SpatialRootAlloHost::update(double dt)
{
    mSession.update();
}

inline void SpatialRootAlloHost::renderAudio(al::AudioIOData& io)
{
    if (!mReady || mPaused) {
        while (io()) {
            for (int c = 0; c < io.channelsOut(); ++c) {
                io.out(c) = 0.0f;
            }
        }
        return;
    }
    mSession.renderExternal(io);
}

inline void SpatialRootAlloHost::setPaused(bool paused)
{
    mPaused = paused;
    mSession.setPaused(paused);
}

inline void SpatialRootAlloHost::setMasterGainDb(float dB)
{
    mSession.setMasterGainDb(dB);
}

inline void SpatialRootAlloHost::setDbapFocus(float focus)
{
    mSession.setDbapFocus(focus);
}

inline void SpatialRootAlloHost::setSpeakerMixDb(float dB)
{
    mSession.setSpeakerMixDb(dB);
}

inline void SpatialRootAlloHost::setSubMixDb(float dB)
{
    mSession.setSubMixDb(dB);
}

inline void SpatialRootAlloHost::setElevationMode(int mode)
{
    mElevationMode = std::max(0, std::min(2, mode));
    mSession.setElevationMode(static_cast<ElevationMode>(mElevationMode));
}

inline void SpatialRootAlloHost::cycleElevationMode()
{
    setElevationMode((mElevationMode + 1) % 3);
}
