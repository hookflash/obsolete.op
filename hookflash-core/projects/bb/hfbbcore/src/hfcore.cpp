/*
 * hfcore.c
 *
 *  Created on: Feb 17, 2013
 *      Author: lawrence.gunn
 */

#include "bbLog.h"
#include <string>

extern "C" void HFCorePrint(const char* text)
{
	std::string fullText = "Hi from the LIBRARY: ";
	fullText += text;
	fullText += "\n";
	Log::WriteToLog(fullText.c_str());
}


