#pragma once
#include <string>
#include <string_view>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <format>

namespace utils
{
	inline constexpr std::string_view GetPluginName() {
		return Plugin::NAME;
	};

	// Return only the folder path
	std::string_view GetPluginFolder();

	std::string_view GetPluginLogFile();

	std::string GetCurrentTimeString(std::string fmt = "%d.%m.%Y %H:%M:%S");
}