#include "AudioEngine.h"

namespace
{
default_random_engine dre;

constexpr int k_adjacentDb = 2;
constexpr int k_testSoundInitialDbfs = -(96 / k_adjacentDb) * k_adjacentDb;
constexpr int k_testSoundMinDbfs = -(128 / k_adjacentDb) * k_adjacentDb;
constexpr double k_testSoundLengthSec = 0.3;
constexpr double k_maxRampLengthSec = 0.1;
constexpr double k_maxRampLengthFraction = 0.2;
constexpr double k_rampAlpha = 4;
constexpr size_t k_numObservationsNeeded = 2;

enum class LeftOrRight {
    left,
    right
};

struct ObservationsWithAudibility {
    size_t inaudibleCount = 0;
    size_t audibleCount = 0;

    void addObservation(bool audible)
    {
        (audible ? audibleCount : inaudibleCount)++;
    }
    size_t totalCount() const
    {
        return inaudibleCount + audibleCount;
    }
};

#if 0
struct Observation {
    InaudibleAndAudibleObservationsAtLevel loudestInaudible;
    AudibleObservationsAtLevel softestAudible;
    Observation() = default;
    bool finished() const
    {
        CHECK(loudestInaudible.totalCount() == 0 || loudestInaudible.inaudibleCount > 0);
        return loudestInaudible.totalCount() >= k_numObservationsNeeded
            && (softestAudible.levelDb > 0 || softestAudible.count >= k_numObservationsNeeded);
    }
    size_t totalCount() const
    {
        return loudestInaudible.totalCount() + softestAudible.count;
    }
    void addObservation(int levelDb, bool audible)
    {
        CHECK(levelDb <= 0);
        CHECK(loudestInaudible.totalCount() == 0 || loudestInaudible.inaudibleCount > 0);
        if (totalCount() == 0) {
            if (audible) {
                loudestInaudible = InaudibleAndAudibleObservationsAtLevel{.levelDb = levelDb - k_adjacentDb};
                softestAudible = AudibleObservationsAtLevel{.levelDb = levelDb, 1};
            } else {
                loudestInaudible = InaudibleAndAudibleObservationsAtLevel{.levelDb = levelDb, .inaudibleCount = 1};
                softestAudible = AudibleObservationsAtLevel{.levelDb = levelDb + k_adjacentDb};
            }
            return;
        }
        if (loudestInaudible.levelDb == levelDb) {
            loudestInaudible.addObservation(audible);
        } else if (softestAudible.levelDb == levelDb) {
            if (audible) {
                softestAudible.count++;
            } else {
                loudestInaudible = InaudibleAndAudibleObservationsAtLevel{
                  .levelDb = levelDb, .inaudibleCount = 1, .audibleCount = softestAudible.count
                };
                softestAudible = AudibleObservationsAtLevel{.levelDb = levelDb + k_adjacentDb};
            }
        } else {
            LOG(FATAL) << fmt::format(
              "Unexpected db ({}), loudestInaudible.db = {}, softestAudible.db = {}",
              levelDb,
              loudestInaudible.levelDb,
              softestAudible.levelDb
            );
        }
    }
    optional<int> nextLevelToTest() const
    {
        if (finished()) {
            return nullopt;
        }
        vector<int> levels;
        if (softestAudible.count < k_numObservationsNeeded && softestAudible.levelDb <= 0) {
            levels.push_back(softestAudible.levelDb);
        }
        if (loudestInaudible.totalCount() < k_numObservationsNeeded) {
            levels.push_back(loudestInaudible.levelDb);
        }
        CHECK(!levels.empty()); // Otherwise, it `finished()` should have been `true`.
        return levels.at(uniform_int_distribution<size_t>(0u, levels.size() - 1)(dre));
    }
};
#endif

namespace TestFreqStatus
{
struct NextLevelToTest {
    int levelDb;
};
struct Finished {
    int loudestInaudible;
    optional<int> softestAudible;
};
using V = variant<NextLevelToTest, Finished>;
} // namespace TestFreqStatus

struct TestFreq {
    LeftOrRight lr;
    double freq;
    std::map<int, ObservationsWithAudibility> os;

    TestFreq(LeftOrRight lrArg, double freqArg)
        : lr(lrArg)
        , freq(freqArg)
    {
    }

    TestFreqStatus::V status() const
    {
        CHECK(!os.empty()); // Phase 1 adds one empty level.
        if (os.size() == 1) {
            auto& kv = *os.begin();
            auto& v = kv.second;
            if (v.totalCount() == 0) {
                // Initially test the result of phase 1.
                return TestFreqStatus::NextLevelToTest{kv.first};
            }
        }

        for (auto level = os.rbegin()->first;;) {
            auto it = os.find(level);
            CHECK(it != os.end());
            if (it->second.inaudibleCount > 0) {
                // This level was inaudible at least once.
                int louderLevel = it->first + k_adjacentDb;
                if (louderLevel > 0) {
                    // Can't go louder.
                    return TestFreqStatus::Finished{.loudestInaudible = it->first};
                } else {
                    auto jt = os.find(louderLevel);
                    if (jt == os.end()) {
                        return TestFreqStatus::NextLevelToTest{louderLevel};
                    } else {
                        CHECK(jt->second.inaudibleCount == 0);
                        if (jt->second.audibleCount == k_numObservationsNeeded) {
                            return TestFreqStatus::Finished{
                              .loudestInaudible = it->first, .softestAudible = louderLevel
                            };
                        } else {
                            return TestFreqStatus::NextLevelToTest{louderLevel};
                        }
                    }
                }
            } else {
                // This level was always audible.
                CHECK(it->second.audibleCount > 0);
                auto softerLevel = level - k_adjacentDb;
                auto jt = os.find(softerLevel);
                if (jt == os.end()) {
                    // Softer level was never tested.
                    return TestFreqStatus::NextLevelToTest{softerLevel};
                } else {
                    CHECK(jt->second.totalCount() > 0);
                    if (jt->second.inaudibleCount == 0) {
                        // Going down until we find some inaudibility.
                        level = softerLevel;
                        continue;
                    } else {
                        if (it->second.audibleCount == k_numObservationsNeeded) {
                            return TestFreqStatus::Finished{.loudestInaudible = softerLevel, .softestAudible = level};
                        } else {
                            return TestFreqStatus::NextLevelToTest{level};
                        }
                    }
                }
            }
            LOG(FATAL) << "unreachable";
        }
    }
};

double gausswin_around_zero_at(size_t n, size_t N, double alpha)
{
    assert(n < N);
    auto L_minus_1 = 2 * (N - 1);
    auto sigma = L_minus_1 / (2 * alpha);
    return exp(-double(n * n) / (2 * sigma * sigma));
}

vector<array<float, 2>> generateClip(double sampleRate, double freq, double dbfs, double lengthSec, LeftOrRight lr)
{
    const double rampLengthSec = std::min(lengthSec * k_maxRampLengthFraction, k_maxRampLengthSec);

    CHECK(2 * rampLengthSec < lengthSec);

    const double amplitude = db2mag(dbfs);
    const auto rampN = size_t(round(sampleRate * rampLengthSec));
    const auto N = size_t(round(sampleRate * lengthSec));
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
        clip[i][size_t(lr)] = float(sin(theta) * amplitude * ramp);
        clip[i][1 - size_t(lr)] = 0;
    }
    return clip;
}

int innerMain(const fs::path& outputPath)
{
    auto ae = AudioEngine::create();

    using clock = chr::high_resolution_clock;
    const auto sampleRate = ae->sampleRate();

    vector<TestFreq> testFreqs;
    for (auto lr : {LeftOrRight::left, LeftOrRight::right}) {
        // testFreqs.push_back(TestFreq(lr, 20.0));
        // testFreqs.push_back(TestFreq(lr, 20.0 * pow(2, 0.25)));
        // testFreqs.push_back(TestFreq(lr, 20.0 * pow(2, 0.5)));
        // testFreqs.push_back(TestFreq(lr, 20.0 * pow(2, 0.75)));
        testFreqs.push_back(TestFreq(lr, 40.0));
        testFreqs.push_back(TestFreq(lr, 80.0));
        testFreqs.push_back(TestFreq(lr, 160.0));
        testFreqs.push_back(TestFreq(lr, 320.0));
        testFreqs.push_back(TestFreq(lr, 640.0));
        testFreqs.push_back(TestFreq(lr, 1280.0));
        testFreqs.push_back(TestFreq(lr, 2560.0));
        testFreqs.push_back(TestFreq(lr, 5120.0));
        testFreqs.push_back(TestFreq(lr, 10240.0));
        // testFreqs.push_back(TestFreq(lr, 10240.0 * pow(2, 0.25)));
        // testFreqs.push_back(TestFreq(lr, 10240.0 * pow(2, 0.5)));
        // testFreqs.push_back(TestFreq(lr, 10240.0 * pow(2, 0.75)));
        // testFreqs.push_back(TestFreq(lr, 20480.0));
    }

    struct FreqAndFreqLeftIxs {
        size_t freqIx;
        size_t freqLeftIx;
    };

    {
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        cbreak();

        printw("%s\n", ae->infoMessage().c_str());
        printw("============\n");
        printw("HEARING TEST\n");
        printw("============\n\n");
        printw("Results will be saved to %s\n\n", outputPath.c_str());
        printw("- Preferably, use closed-back headphones\n");
        printw("- Turn up all volume controls on your computer / audio device\n\n");
        printf("Using the LEFT and RIGHT arrow keys set the volume to loudest level you still can't hear then press "
               "ENTER.\n\n");
        int volrow, volcol;
        getyx(stdscr, volrow, volcol);

        auto startNextSoundAt = clock::time_point::min();
        int testSoundLevelDb = k_testSoundInitialDbfs;
        size_t previousBarLength = SIZE_MAX;
        static_assert(k_testSoundMinDbfs < 0);
        const auto fullBarLength = size_t(round(-k_testSoundMinDbfs));
        optional<FreqAndFreqLeftIxs> freqIxs;
        auto testFreqsLeft = vi::iota(0u, testFreqs.size() / 2) | ra::to<vector<size_t>>();
        auto lr = LeftOrRight::left;
        while (!testFreqsLeft.empty()) {
            if (!freqIxs) {
                auto freqLeftIx = uniform_int_distribution<size_t>(0u, testFreqsLeft.size() - 1)(dre);
                auto freqIx = testFreqsLeft.at(freqLeftIx);
                freqIxs = FreqAndFreqLeftIxs{.freqIx = freqIx, .freqLeftIx = freqLeftIx};
            }
            auto& tf = testFreqs.at(freqIxs->freqIx);
            auto now = clock::now();
            // If sound is done playing, play again
            if (startNextSoundAt <= now) {
                ae->addClip(generateClip(sampleRate, tf.freq, testSoundLevelDb, k_testSoundLengthSec, lr));
                startNextSoundAt =
                  now + chr::duration_cast<clock::duration>(chr::duration<double>(k_testSoundLengthSec));
                lr = LeftOrRight(1 - int(lr));
            }
            // Check last kbhit
            bool sleep = false;
            bool enterPressed = false;
            switch (getch()) {
            case KEY_LEFT:
                testSoundLevelDb = std::max(testSoundLevelDb - k_adjacentDb, k_testSoundMinDbfs);
                break;
            case KEY_RIGHT:
                testSoundLevelDb = std::min(testSoundLevelDb + k_adjacentDb, 0);
                break;
            case '\n':
                enterPressed = true;
                break;
            default:
                sleep = true;
            }
            // Print volume bar.
            auto barLength = size_t(testSoundLevelDb - k_testSoundMinDbfs);
            if (previousBarLength != barLength) {
                auto s = std::to_string(testSoundLevelDb);
                s = string(std::max<size_t>(3, s.size()) - s.size(), ' ') + s;
                mvprintw(
                  volrow,
                  volcol,
                  "Phase 1, testing frequency %zu of %zu\n",
                  testFreqs.size() - testFreqsLeft.size() + 1,
                  testFreqs.size()
                );
                printw(
                  "Testing %.0f Hz / %s dBFS [%s%s]    \n",
                  tf.freq,
                  s.c_str(),
                  std::string(barLength, '=').c_str(),
                  std::string(fullBarLength - barLength, ' ').c_str()
                );
                refresh();
                previousBarLength = barLength;
            }
            if (sleep) {
                this_thread::sleep_for(chr::milliseconds(10));
            }
            if (enterPressed) {
                tf.os[testSoundLevelDb];
                testFreqs.at(freqIxs->freqIx + testFreqs.size() / 2).os[testSoundLevelDb];
                testFreqsLeft.erase(testFreqsLeft.begin() + ptrdiff_t(freqIxs->freqLeftIx));
                freqIxs.reset();
                testSoundLevelDb = k_testSoundInitialDbfs;
            }
        }
    }

    auto testFreqsLeft = vi::iota(0u, testFreqs.size()) | ra::to<vector<size_t>>();

    while (!testFreqsLeft.empty()) {
        FreqAndFreqLeftIxs freqIxs;
        {
            auto freqLeftIx = uniform_int_distribution<size_t>(0u, testFreqsLeft.size() - 1)(dre);
            auto freqIx = testFreqsLeft.at(freqLeftIx);
            freqIxs = FreqAndFreqLeftIxs{.freqIx = freqIx, .freqLeftIx = freqLeftIx};
        }
        auto& tf = testFreqs.at(freqIxs.freqIx);

        auto tfstatus = tf.status();
        auto* x = std::get_if<TestFreqStatus::NextLevelToTest>(&tfstatus);
        CHECK(x);
        auto dbToTest = x->levelDb;

        clear();
        printw("Phase 2, %zu frequencies to go out of %zu\n", testFreqsLeft.size(), testFreqs.size());
        printw("Testing %f / %d dB, %s\n\n", tf.freq, dbToTest, tf.lr == LeftOrRight::left ? "left" : "right");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
        printw("Here comes the sound...");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
        ae->addClip(generateClip(sampleRate, tf.freq, dbToTest, k_testSoundLengthSec, tf.lr));
        this_thread::sleep_for(chr::duration<double>(k_testSoundLengthSec));
        this_thread::sleep_for(chr::milliseconds(50));
        printw("done.\n");
        refresh();
        this_thread::sleep_for(chr::milliseconds(100));

        printw("Did you hear it? Press LEFT for left ear, RIGHT for right ear and SPACE if you didn't\n");
        refresh();

        bool done = false;
        bool audible = false;

        for (; !done;) {
            switch (getch()) {
            case KEY_LEFT:
                audible = tf.lr == LeftOrRight::left;
                done = true;
                break;
            case KEY_RIGHT:
                audible = tf.lr == LeftOrRight::right;
                done = true;
                break;
            case ' ':
                done = true;
                break;
            default:
                this_thread::sleep_for(chr::milliseconds(10));
            }
        }
        tf.os[dbToTest].addObservation(audible);
        if (holds_alternative<TestFreqStatus::Finished>(tf.status())) {
            testFreqsLeft.erase(testFreqsLeft.begin() + ptrdiff_t(freqIxs.freqLeftIx));
        }
        printw("OK!\n");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
    }

    std::map<double, pair<double, double>> result;
    for (auto& tf : testFreqs) {
        result[tf.freq] = pair(NAN, NAN);
    }
    for (auto& tf : testFreqs) {
        auto tfstatus = tf.status();
        auto* finished = std::get_if<TestFreqStatus::Finished>(&tfstatus);
        CHECK(finished);
        if (!finished->softestAudible) {
            continue;
        }
        switch (tf.lr) {
        case LeftOrRight::left:
            result[tf.freq].first = *finished->softestAudible;
            break;
        case LeftOrRight::right:
            result[tf.freq].second = *finished->softestAudible;
            break;
        }
    }

    std::ofstream of(outputPath);
    of << fmt::format("x = [\n");
    for (auto& kv : result) {
        of << fmt::format("\t{}\n", kv.first);
    }
    of << fmt::format("];\n");
    of << fmt::format("y = [\n");
    for (auto& kv : result) {
        of << fmt::format("\t{} {}\n", kv.second.first, kv.second.second);
    }
    of << fmt::format("];\n");
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char* argv[])
{
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

    LOG_IF(FATAL, argc != 2) << fmt::format("Usage: `{} <output-path>`", fs::path(argv[0]).filename());

#if 0
    auto c=    generateClip(96000, 440, -100, 1, LeftOrRight::left);
    array<float,2>mins,maxs;
    mins.fill(INFINITY);
    maxs.fill(-INFINITY);
    for(auto &x:c){
        for(size_t j:{0u,1u}){
            mins[j] = std::min(mins[j], x[j]);
            maxs[j] = std::max(maxs[j], x[j]);
        }
    }
    fmt::println("mins: {} and {}", mins[0], mins[1]);
    fmt::println("maxs: {} and {}", maxs[0], maxs[1]);
#endif

    auto outputPath = fs::path(argv[1]);

    initscr();
    auto result = innerMain(outputPath);
    endwin();

    return result;
}
