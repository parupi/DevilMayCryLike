#include "Logger.h"
#include <dxgidebug.h>

namespace Logger {
	void Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}
}
