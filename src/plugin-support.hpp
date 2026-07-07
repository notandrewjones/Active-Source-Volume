/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <obs.h>

#define PLUGIN_NAME "active-source-volume"
#define PLUGIN_VERSION "1.3.0"
#define DOCK_ID "active-source-volume-dock"
#define VENDOR_NAME "active-source-volume"

/*
 * Logs through libobs' blog() with a consistent "[active-source-volume]" prefix.
 * level is one of LOG_ERROR / LOG_WARNING / LOG_INFO / LOG_DEBUG.
 */
void plog(int level, const char *format, ...);
