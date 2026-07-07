/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "plugin-support.hpp"

#include <cstdarg>
#include <cstdio>

void plog(int level, const char *format, ...)
{
	char msg[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	blog(level, "[%s] %s", PLUGIN_NAME, msg);
}
