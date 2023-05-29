#pragma once
#include <string>

namespace MoeSSLogger
{
	class Logger
	{
	public:
		Logger();
		void log(const std::wstring&) const;
		void error(const std::wstring&) const;
	};
}

static  MoeSSLogger::Logger logger;