namespace fs = std::filesystem;

int main(int argc, const char* argv[])
{
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    fmt::println("Test stdout");
    LOG(INFO) << "Test LOG(INFO)";
    LOG_IF(FATAL, argc != 2) << fmt::format("Usage: `{} <output-path>`", fs::path(argv[0]).filename());
}
