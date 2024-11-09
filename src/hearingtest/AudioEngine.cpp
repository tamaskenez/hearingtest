#include "AudioEngine.h"

namespace
{
constexpr size_t k_maxNumTracks = 2;

struct ClipWithPlayHead {
    explicit ClipWithPlayHead(vector<array<float, 2>>&& clipArg)
        : clip(MOVE(clipArg))
    {
    }
    ClipWithPlayHead(const ClipWithPlayHead&) = delete;

    vector<array<float, 2>> clip;
    size_t playHead = 0;
};

class AudioIODeviceCallback : public juce::AudioIODeviceCallback
{
public:
    AudioIODeviceCallback()
    {
        audioThreadClips.fill(nullptr);
    }
    ~AudioIODeviceCallback() override
    {
        for (auto& c : inputClips) {
            delete c.load();
        }
        for (auto& c : outputClips) {
            delete c.load();
        }
        for (auto& c : audioThreadClips) {
            delete c;
        }
    }
    void addClip(vector<array<float, 2>>&& clip)
    {
        for (auto& outClip : outputClips) {
            auto* pc = outClip.exchange(nullptr);
            delete pc;
        }
        auto* pc = new ClipWithPlayHead(MOVE(clip));
        for (auto& inClip : inputClips) {
            pc = inClip.exchange(pc);
            if (!pc) {
                return;
            }
        }
        LOG(FATAL) << "addClip: No free input clip found.";
    }

private:
    void audioDeviceIOCallbackWithContext(
      UNUSED const float* const* inputChannelData,
      UNUSED int numInputChannels,
      float* const* outputChannelData,
      int numOutputChannels,
      int numSamples,
      UNUSED const juce::AudioIODeviceCallbackContext& context
    ) override
    {
        for (auto& inClip : inputClips) {
            if (auto* pc = inClip.exchange(nullptr)) {
                moveInClip(pc);
            }
        }
        for (int ch = 0; ch < numOutputChannels; ++ch) {
            for (int s = 0; s < numSamples; ++s) {
                outputChannelData[ch][s] = 0.0f;
            }
        }
        for (auto& clip : audioThreadClips) {
            if (!clip) {
                continue;
            }
            auto playHead = clip->playHead;
            assert(playHead <= clip->clip.size());
            auto samplesToPlay = std::min(clip->clip.size() - playHead, size_t(numSamples));
            for (size_t ch = 0; ch < std::min<size_t>(size_t(numOutputChannels), 2); ++ch) {
                for (size_t s = 0; s < samplesToPlay; ++s) {
                    outputChannelData[ch][s] += clip->clip[playHead + s][ch];
                }
            }
            clip->playHead += samplesToPlay;
            if (clip->playHead >= clip->clip.size()) {
                moveOutClip(&clip);
            }
        }
    }
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        LOG(INFO) << fmt::format(
          "Audio device {} ({}) is about to start", device->getName().toStdString(), device->getTypeName().toStdString()
        );
    }
    void audioDeviceStopped() override
    {
        LOG(INFO) << "Audio device was stopped";
    }
    void audioDeviceError(const juce::String& errorMessage) override
    {
        LOG(FATAL) << fmt::format("AudioIODeviceCallback::audioDeviceError: {}", errorMessage.toStdString());
    }

    void moveInClip(ClipWithPlayHead* pc)
    {
        for (auto& audioThreadClip : audioThreadClips) {
            if (!audioThreadClip) {
                audioThreadClip = pc;
                return;
            }
        }
        LOG(FATAL) << "moveInClip: No free audio thread clip found.";
    }
    void moveOutClip(ClipWithPlayHead** ppClip)
    {
        for (auto& outClip : outputClips) {
            *ppClip = outClip.exchange(*ppClip);
            if (!*ppClip) {
                return;
            }
        }
        CHECK(!*ppClip);
    }

    array<atomic<ClipWithPlayHead*>, k_maxNumTracks> inputClips;
    array<ClipWithPlayHead*, k_maxNumTracks> audioThreadClips;
    array<atomic<ClipWithPlayHead*>, 2 * k_maxNumTracks> outputClips;
};
} // namespace

struct AudioEngineImpl : AudioEngine {
    AudioEngineImpl()
    {
        auto admResult = deviceManager.initialiseWithDefaultDevices(0, 2);
        LOG_IF(FATAL, admResult.isNotEmpty())
          << fmt::format("audioDeviceManager.initialiseWithDefaultDevices returned: {}", admResult.toStdString());
        deviceManager.addAudioCallback(&aiodc);
    }
    void addClip(vector<array<float, 2>>&& clip) override
    {
        aiodc.addClip(MOVE(clip));
    }
    double sampleRate() const override
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        return setup.sampleRate;
    }

    AudioIODeviceCallback aiodc;
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    juce::AudioDeviceManager deviceManager;
};

unique_ptr<AudioEngine> AudioEngine::create()
{
    return make_unique<AudioEngineImpl>();
}
