#pragma once
#include <string>
#include "../StringPreprocess.hpp"
#define __MOEVS_DEBUG_MESSAGE(msg) __MOEVS_DEBUG_INFO(__FILE__, __LINE__, msg);
#define logger MoeSSLogger::GetLogger()
inline std::string __MOEVS_DEBUG_INFO(const char* filename, int line, const char* msg)
{
	return std::string("[Debug] ") + filename + ":" + std::to_string(line) + " | " + msg;
}

inline std::wstring __MOEVS_DEBUG_INFO(const char* filename, int line, const wchar_t* msg)
{
	return L"[Debug] " + to_wide_string(filename) + L":" + std::to_wstring(line) + L" | " + msg;
}

namespace MoeSSLogger
{
	class Logger
	{
	public:
		Logger();
		void log(const std::wstring&) const;
		void error(const std::wstring&) const;
	};

	Logger& GetLogger();
}