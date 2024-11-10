#include "AudioEngine.h"

namespace
{
default_random_engine dre;

constexpr double k_testSoundInitialDbfs = -32;
constexpr double k_testSoundMinDbfs = -64;
constexpr double k_testSoundFreq = 440;
constexpr double k_testSoundLengthSec = 0.5;
constexpr double k_maxRampLengthSec = 0.1;
constexpr double k_maxRampLengthFraction = 0.2;
constexpr double k_rampAlpha = 4;
constexpr int k_adjacentDb = 1;
constexpr int k_nonadjacentDbStep = 5;

enum class LeftOrRight {
    left,
    right
};

struct InaudibleAndAudibleCounts {
    static InaudibleAndAudibleCounts createSingleObservation(bool audible)
    {
        return InaudibleAndAudibleCounts{audible ? 0u : 1u, audible ? 1u : 0u};
    }
    void addObservation(bool audible)
    {
        (audible ? audibleCount : inaudibleCount)++;
    }
    size_t totalCount() const
    {
        return inaudibleCount + audibleCount;
    }
    bool clearlyAudible() const
    {
        return audibleCount > inaudibleCount;
    }
    bool clearlyInaudible() const
    {
        return inaudibleCount > audibleCount;
    }
    size_t inaudibleCount = 0;
    size_t audibleCount = 0;
};

namespace Observation
{
struct NotTested {
};
struct SingleLevel {
    int db;
    bool audible;
};
struct Bracket {
    std::map<int, InaudibleAndAudibleCounts> counts;
    optional<int> simplify()
    {
        if (counts.size() >= 2) {
            auto maxCount =
              ra::max_element(counts, {}, [](const std::map<int, InaudibleAndAudibleCounts>::value_type& kv) -> size_t {
                  return kv.second.totalCount();
              })->second.totalCount();
            for (auto& kv : counts) {
                auto& s = kv.second;
                if (s.totalCount() == maxCount) {
                    continue;
                }
                if (s.clearlyAudible() && s.audibleCount > s.inaudibleCount + maxCount - s.totalCount()) {
                    // Still clearly audible even if all subsequent observations are inaudible.
                    s.inaudibleCount += maxCount - s.totalCount();
                    continue;
                }
                if (s.clearlyInaudible() && s.inaudibleCount > s.audibleCount + maxCount - s.totalCount()) {
                    // Still clearly inaudible even if all subsequent observations are audible.
                    s.audibleCount += maxCount - s.totalCount();
                    continue;
                }
            }
            // Remove highest level audible observations if they're complete and next highest is also audible and
            // complete.
            while (counts.size() >= 2) {
                auto highest = counts.rbegin();
                auto nextHighest = std::next(highest);
                if(highest->second.totalCount()==maxCount&&highest->second.clearlyAudible()&&
                   nextHighest->second.totalCount()==maxCount&&nextHighest->second.clearlyAudible()){
                    counts.erase(highest.base());
                } else {
                    break;
                }
            }
            // Remove lower level inaudble observations if they're complete and next lowest level is also inaudible and
            // complete..
            while (counts.size() >= 2) {
                auto lowest = counts.begin();
                auto nextLowest = std::next(lowest);
                if(lowest->second.totalCount()==maxCount&&lowest->second.clearlyInaudible()&&
                   nextLowest->second.totalCount()==maxCount&&nextLowest->second.clearlyInaudible()){
                    counts.erase(lowest);
                } else {
                    break;
                }
            }

            // Return finished db if conditions are met.
            if (counts.size() == 2) {
                auto it = counts.begin();
                auto jt = std::next(it);
                if(jt->first - it->first <= k_adjacentDb && it->second.totalCount()==jt->second.totalCount()&&it->second.clearlyInaudible()&&jt->second.clearlyAudible()){
                    return jt->first;
                }
            }
        }
        return std::nullopt;
    }
};
struct FinishedTest {
    optional<int> softestAudibleDb;
};
using V = variant<NotTested, SingleLevel, Bracket, FinishedTest>;
} // namespace Observation

struct TestFreq {
    LeftOrRight lr;
    double freq;
    Observation::V observation = Observation::NotTested{};
    void addObservation(int db, bool audible)
    {
        CHECK(db <= 0);

        if (db == 0 && !audible) {
            // Highest level not audible.
            observation = Observation::FinishedTest{};
            return;
        }

        switch_variant(
          MOVE(observation),
          [&, this](Observation::NotTested) {
              observation = Observation::SingleLevel{db, audible};
          },
          [&, this](Observation::SingleLevel x) {
              CHECK(db != x.db); // Should not test for same db.
              if (audible == x.audible) {
                  if (audible) {
                      // 2 audible observation, keep the lower level.
                      observation = Observation::SingleLevel{std::min(x.db, db), true};
                  } else {
                      // 2 inaudible observations, keep the higher level.
                      observation = Observation::SingleLevel{std::max(x.db, db), false};
                  }
              } else {
                  std::map<int, InaudibleAndAudibleCounts> m = {
                    {x.db, InaudibleAndAudibleCounts::createSingleObservation(x.audible)},
                    {db,   InaudibleAndAudibleCounts::createSingleObservation(audible)  }
                  };
                  observation = Observation::Bracket{MOVE(m)};
              }
          },
          [&, this](Observation::Bracket x) {
              x.counts[db].addObservation(audible);
              if (auto finalDb = x.simplify()) {
                  observation = Observation::FinishedTest{*finalDb};
              } else {
                  observation = MOVE(x);
              }
          },
          [](Observation::FinishedTest) {
              LOG(FATAL) << "Observation received for finished test";
          }
        );
    }

    int nextDbToTest(int initialDb) const
    {
        return switch_variant(
          observation,
          [&](Observation::NotTested) {
              return initialDb;
          },
          [&](Observation::SingleLevel x) {
              if (x.audible) {
                  return x.db - k_nonadjacentDbStep;
              } else {
                  CHECK(x.db < 0);
                  return std::min(x.db + k_nonadjacentDbStep, 0);
              }
          },
          [&](const Observation::Bracket& x) {
              CHECK(!x.counts.empty());
              if (x.counts.size() == 1) {
                  auto it = x.counts.begin();
                  if (it->second.clearlyAudible()) {
                      return it->first - k_adjacentDb;
                  } else if (it->second.clearlyInaudible()) {
                      CHECK(it->first < 0);
                      return std::min(it->first + k_adjacentDb, 0);
                  } else {
                      return it->first;
                  }
              }
              // Binary search if conditions are correct.
              if (x.counts.size() == 2) {
                  auto it = x.counts.begin();
                  auto jt = std::next(it);
                  if (jt->first - it->first > k_adjacentDb && it->second.clearlyInaudible() && jt->second.clearlyAudible()) {
                      return std::midpoint(it->first, jt->first);
                  }
              }
              // Pick random level among least represented ones.
              auto minCount = ra::min_element(
                                x.counts,
                                {},
                                [](const std::map<int, InaudibleAndAudibleCounts>::value_type& kv) -> size_t {
                                    return kv.second.totalCount();
                                }
              )->second.totalCount();
              vector<int> dbsWithMinCount;
              for (auto& kv : x.counts) {
                  if (kv.second.totalCount() == minCount) {
                      dbsWithMinCount.push_back(kv.first);
                  }
              }
              CHECK(!dbsWithMinCount.empty());
              return dbsWithMinCount.at(uniform_int_distribution<size_t>(0, dbsWithMinCount.size() - 1)(dre));
          },
          [](Observation::FinishedTest) -> int {
              LOG(FATAL) << "Observation received for finished test";
          }
        );
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
    auto startNextSoundAt = clock::time_point::min();
    double testSoundDbfs = k_testSoundInitialDbfs;
    const auto sampleRate = ae->sampleRate();
    {
        printw("%s\n", ae->infoMessage().c_str());
        printw("\n--------\n");
        printw("Starting hearing test. Results will be saved to %s\n\n", outputPath.c_str());
        printw("- Preferably, use closed-back headphones\n");
        printw("- Turn up all volume controls on your computer / audio device\n");
        printw("- Using the left and right arrow keys set the volume to a very soft but clearly audible level.\n");
        printw("- Then, press enter to start the test\n\n");

        LeftOrRight lr = LeftOrRight::left;

        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        cbreak();

        size_t previousBarLength = SIZE_MAX;
        static_assert(k_testSoundMinDbfs < 0);
        const auto fullBarLength = size_t(round(-k_testSoundMinDbfs));
        for (;;) {
            auto now = clock::now();
            // If sound is done playing, play again
            if (startNextSoundAt <= now) {
                ae->addClip(generateClip(sampleRate, k_testSoundFreq, testSoundDbfs, k_testSoundLengthSec, lr));
                startNextSoundAt =
                  now + chr::duration_cast<clock::duration>(chr::duration<double>(k_testSoundLengthSec));
                lr = LeftOrRight(1 - int(lr));
            }
            // Check last kbhit
            bool sleep = false;
            bool enterPressed = false;
            switch (getch()) {
            case KEY_LEFT:
                testSoundDbfs = std::max(testSoundDbfs - 1, k_testSoundMinDbfs);
                break;
            case KEY_RIGHT:
                testSoundDbfs = std::min(testSoundDbfs + 1, 0.0);
                break;
            case '\n':
                enterPressed = true;
                break;
            default:
                sleep = true;
            }
            if (enterPressed) {
                break;
            }
            // Print volume bar.
            auto barLength = size_t(round(testSoundDbfs - k_testSoundMinDbfs));
            if (previousBarLength != barLength) {
                auto s = std::to_string(int(round(testSoundDbfs)));
                s = string(std::max<size_t>(5, s.size()) - s.size(), ' ') + s;
                printw(
                  "Volume: %s dBFS [%s%s]\r",
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
        }
    }

    const int initialDb = int(round(testSoundDbfs));
    vector<TestFreq> testFreqs;
    for (auto lr : {LeftOrRight::left, LeftOrRight::right}) {
        testFreqs.push_back(TestFreq{lr, 20.0});
        testFreqs.push_back(TestFreq{lr, 20.0 * pow(2, 0.25)});
        testFreqs.push_back(TestFreq{lr, 20.0 * pow(2, 0.5)});
        testFreqs.push_back(TestFreq{lr, 20.0 * pow(2, 0.75)});
        testFreqs.push_back(TestFreq{lr, 40.0});
        testFreqs.push_back(TestFreq{lr, 80.0});
        testFreqs.push_back(TestFreq{lr, 160.0});
        testFreqs.push_back(TestFreq{lr, 320.0});
        testFreqs.push_back(TestFreq{lr, 640.0});
        testFreqs.push_back(TestFreq{lr, 1280.0});
        testFreqs.push_back(TestFreq{lr, 2560.0});
        testFreqs.push_back(TestFreq{lr, 5120.0});
        testFreqs.push_back(TestFreq{lr, 10240.0});
        testFreqs.push_back(TestFreq{lr, 10240.0 * pow(2, 0.25)});
        testFreqs.push_back(TestFreq{lr, 10240.0 * pow(2, 0.5)});
        testFreqs.push_back(TestFreq{lr, 10240.0 * pow(2, 0.75)});
        testFreqs.push_back(TestFreq{lr, 20480.0});
    }
    auto testFreqsLeft = vi::iota(0u, testFreqs.size()) | ra::to<vector<size_t>>();
    while (!testFreqsLeft.empty()) {
        auto idx = uniform_int_distribution<size_t>(0u, testFreqsLeft.size() - 1)(dre);
        auto tidx = testFreqsLeft[idx];
        auto& tf = testFreqs[tidx];
        auto dbToTest = tf.nextDbToTest(initialDb);

        clear();
        printw("Testing %f / %d dB\n", tf.freq, dbToTest);
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
        printw("Here comes the sound...");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
        ae->addClip(generateClip(sampleRate, tf.freq, dbToTest, k_testSoundLengthSec, tf.lr));
        this_thread::sleep_for(chr::duration<double>(k_testSoundLengthSec));
        this_thread::sleep_for(chr::milliseconds(300));
        printw("done.\n");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));

        printw("Did you hear it? Press 'A' for left ear, 'L' for right ear and SPACE if you didn't\n");
        refresh();

        bool done = false;
        bool audible = false;

        for (; !done;) {
            switch (getch()) {
            case 'a':
                audible = tf.lr == LeftOrRight::left;
                done = true;
                break;
            case 'l':
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
        tf.addObservation(dbToTest, audible);
        if (holds_alternative<Observation::FinishedTest>(tf.observation)) {
            testFreqsLeft.erase(testFreqsLeft.begin() + ptrdiff_t(idx));
        }
        printw("OK!\n");
        refresh();
        this_thread::sleep_for(chr::milliseconds(300));
    }
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char* argv[])
{
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

    LOG_IF(FATAL, argc != 2) << fmt::format("Usage: `{} <output-path>`", fs::path(argv[0]).filename());

    auto outputPath = fs::path(argv[1]);

    initscr();
    auto result = innerMain(outputPath);
    endwin();

    return result;
}
