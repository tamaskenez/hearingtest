#include "AudioEngine.h"

namespace
{
vector<array<float, 2>> generateClip(double sampleRate, double lengthSec, double freq)
{
    auto N = size_t(round(sampleRate * lengthSec));
    vector<array<float, 2>> clip(N);
    for (size_t i = 0; i < N; ++i) {
        auto theta = 2 * numbers::pi * i / sampleRate * freq;
        clip[i][0] = float(sin(theta)) / 100;
        clip[i][1] = 0.0f;
    }
    return clip;
}
} // namespace

int main(int argc, const char* argv[])
{
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    fmt::println("Test stdout");
    LOG(INFO) << "Test LOG(INFO)";
    LOG_IF(FATAL, argc != 2) << fmt::format("Usage: `{} <output-path>`", fs::path(argv[0]).filename());

    auto outputPath = fs::path(argv[1]);

    LOG(INFO) << fmt::format("Output path: {}", outputPath);

    auto ae = AudioEngine::create();

    LOG(INFO) << "Sleeping 1 secs.";
    this_thread::sleep_for(chr::seconds(1));
    LOG(INFO) << "Add clip";
    ae->addClip(generateClip(ae->sampleRate(), 1.0, 440.0));
    LOG(INFO) << "Sleeping 2 secs.";
    this_thread::sleep_for(chr::seconds(2));
    LOG(INFO) << "Done sleeping.";
}
