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
namespace ra = std::ranges;
namespace vi = std::views;

using std::array;
using std::atomic;
using std::default_random_engine;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::pair;
using std::string;
using std::string_view;
using std::uniform_int_distribution;
using std::unique_ptr;
using std::variant;
using std::vector;

inline double db2mag(double db)
{
    return pow(10, db / 20);
}

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename... Ts, typename Variant>
auto switch_variant(Variant&& variant, Ts&&... ts)
{
    return std::visit(overloaded{std::forward<Ts>(ts)...}, std::forward<Variant>(variant));
}
