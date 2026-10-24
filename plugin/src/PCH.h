#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace logger = SKSE::log;
