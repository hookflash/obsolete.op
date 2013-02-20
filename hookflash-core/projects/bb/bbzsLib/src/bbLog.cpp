#include "bbLog.h"
#include "stdio.h"
#include "wchar.h"

void Log::WriteToLog(const std::string& message)
{
	std::string fullMessage = "LOG: ";
	fullMessage += message;
	fullMessage += "\n";
	fprintf(stderr, fullMessage.c_str());
}

