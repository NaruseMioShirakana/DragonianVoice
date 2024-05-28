#pragma once
#include <string>
#include "../StringPreprocess.hpp"
#include <filesystem>
#include <mutex>
#define __MOEVS_DEBUG_MESSAGE(msg) __MOEVS_DEBUG_INFO(__FILE__, __LINE__, msg)
#define logger MoeSSLogger::GetLogger()
inline std::string __MOEVS_DEBUG_INFO(const char* filename, int line, const char* msg)
{
	return std::string("[In \"") + std::filesystem::path(filename).filename().string() + "\" Line " + std::to_string(line) + "] " + msg;
}

inline std::wstring __MOEVS_DEBUG_INFO(const char* filename, int line, const wchar_t* msg)
{
	return std::wstring(L"[In \"") + std::filesystem::path(filename).filename().wstring() + L"\" Line " + std::to_wstring(line) + L"] " + msg;
}

namespace MoeSSLogger
{
	class Logger
	{
	public:
		Logger();
		~Logger();
		void log(const std::wstring&);
		void log(const char*);
		void error(const std::wstring&);
		void error(const char*);
	private:
		std::filesystem::path cur_log_dir, logpath, errorpath;
		FILE* log_file = nullptr,* error_file = nullptr;
		std::mutex mx;
	};

	Logger& GetLogger();
}