#include "AudioEngine.h"

namespace
{

double gausswin_around_zero_at(size_t n, size_t N, double alpha)
{
    assert(n < N);
    auto L_minus_1 = 2 * (N - 1);
    auto sigma = L_minus_1 / (2 * alpha);
    return exp(-double(n * n) / (2 * sigma * sigma));
}

vector<array<float, 2>> generateClip(double sampleRate, double freq, double dbfs)
{
    constexpr double k_lengthSec = 0.5;
    constexpr double k_rampLengthSec = 0.1;
    constexpr double k_rampAlpha = 4;

    CHECK(2 * k_rampLengthSec < k_lengthSec);

    const double amplitude = db2mag(dbfs);
    const auto rampN = size_t(round(sampleRate * k_rampLengthSec));
    const auto N = size_t(round(sampleRate * k_lengthSec));
    vector<array<float, 2>> clip(N);
    for (size_t i = 0; i < N; ++i) {
        auto theta = 2 * numbers::pi * i / sampleRate * freq;
        double ramp = 1.0;
        if (i < rampN) {
            ramp = gausswin_around_zero_at(rampN - 1 - i, rampN, k_rampAlpha);
        } else if (N - rampN <= i) {
            ramp = gausswin_around_zero_at(i + rampN - N, rampN, k_rampAlpha);
        } else {
            NOP;
        }
        clip[i][0] = float(sin(theta) * amplitude * ramp);
        clip[i][1] = float(ramp);
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
    ae->addClip(generateClip(ae->sampleRate(), 440.0, 0.0));
    LOG(INFO) << "Sleeping 2 secs.";
    this_thread::sleep_for(chr::seconds(2));
    LOG(INFO) << "Done sleeping.";
}
