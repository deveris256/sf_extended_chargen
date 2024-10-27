#include "LogWrapper.h"

namespace logger
{
	std::shared_ptr<spdlog::logger> g_logger = spdlog::basic_logger_mt("logger", utils::GetPluginLogFile().data(), true);

	void c_printf(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		RE::ConsoleLog::GetSingleton()->Print(fmt, args);
		va_end(args);
	}
}