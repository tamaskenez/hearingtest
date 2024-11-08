#include "fmt/std.h"

int main()
{
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    fmt::println("Start");
}
