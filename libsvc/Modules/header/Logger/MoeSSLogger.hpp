#pragma once
#include <string>
#include "../StringPreprocess.hpp"
#include "../../framework.h"
#include <filesystem>
#include <mutex>
#define __MOEVS_DEBUG_MESSAGE(msg) __MOEVS_DEBUG_INFO(__FILE__, __LINE__, msg)
#define logger MoeSSLogger::GetLogger()

namespace MoeSSLogger
{
	class Logger
	{
	public:
		Logger();
		~Logger();
		LibSvcApi void log(const std::wstring&);
		LibSvcApi void log(const char*);
		LibSvcApi void error(const std::wstring&);
		LibSvcApi void error(const char*);
	private:
		std::filesystem::path cur_log_dir, logpath, errorpath;
		FILE* log_file = nullptr,* error_file = nullptr;
		std::mutex mx;
	};

	LibSvcApi Logger& GetLogger();
}