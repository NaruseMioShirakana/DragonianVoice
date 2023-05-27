#pragma once
#include <string>

namespace MoeSSLogger
{
	class Logger
	{
	public:
		Logger();
		void log(const std::wstring&);
		void error(const std::wstring&);
	};
}

static  MoeSSLogger::Logger logger;