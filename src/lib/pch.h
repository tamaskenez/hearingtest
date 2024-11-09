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

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numbers>
#include <thread>
#include <utility>
#include <vector>
