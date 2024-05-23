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
		using logger_fn = void(*)(const wchar_t*, const char*);
		Logger();
		~Logger();
		Logger(logger_fn error_fn, logger_fn log_fn);
		void log(const std::wstring&);
		void log(const char*);
		void error(const std::wstring&);
		void error(const char*);
		void enable(bool _filelogger)
		{
			filelogger = _filelogger;
		}
	private:
		bool custom_logger_fn = false;
		std::filesystem::path cur_log_dir, logpath, errorpath;
		logger_fn cerror_fn = nullptr, cloggerfn = nullptr;
		FILE* log_file = nullptr, * error_file = nullptr;
		bool filelogger = true;
		std::mutex mx;
	};

	LibSvcApi Logger& GetLogger();
}