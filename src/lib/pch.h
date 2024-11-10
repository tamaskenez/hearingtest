#pragma once

#ifndef __cplusplus
  #error Some C file is being compiled in the project.
#endif

#include "absl/cleanup/cleanup.h"
#include "absl/log/check.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "fmt/std.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
