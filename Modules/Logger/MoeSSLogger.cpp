#include "MoeSSLogger.hpp"
#include "../StringPreprocess.hpp"
#include <iostream>

namespace MoeSSLogger
{
	Logger::Logger() = default;

	void Logger::log(const std::wstring& format) const
	{
		std::cout << to_byte_string(format) + '\n';
	}

	void Logger::error(const std::wstring& format) const
	{
		std::cout<< "[ERROR] " << to_byte_string(format) + '\n';
	}

	static MoeSSLogger::Logger MoeVsLogger;

	Logger& GetLogger()
	{
		return MoeVsLogger;
	}
}
