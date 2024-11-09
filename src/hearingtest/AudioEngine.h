#pragma once

class AudioEngine
{
public:
    static unique_ptr<AudioEngine> create();
    virtual ~AudioEngine() = default;

    virtual double sampleRate() const = 0;
    virtual void addClip(vector<array<float, 2>>&& clip) = 0;
};
