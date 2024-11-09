#pragma once

#include "pch.h"

inline void __inline_void_function_with_empty_body__() {}

#define UNUSED [[maybe_unused]]
#define NOP __inline_void_function_with_empty_body__()
#define MOVE(x) std::move(x)

namespace fs = std::filesystem;
namespace this_thread = std::this_thread;
namespace chr = std::chrono;
namespace numbers = std::numbers;

using std::array;
using std::atomic;
using std::make_unique;
using std::unique_ptr;
using std::vector;

inline double db2mag(double db)
{
    return pow(10, db / 20);
}
