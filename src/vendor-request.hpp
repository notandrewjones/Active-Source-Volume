/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

class ActiveBrowser;

/*
 * obs-websocket vendor requests (vendor name: "active-source-volume"):
 *
 *   Selected source:
 *     NudgeVolume  { deltaDb }  -> { sourceName, db, muted, hasSource }
 *     ToggleMute                -> { sourceName, db, muted, hasSource }
 *     GetControlled             -> { sourceName, db, muted, hasSource }
 *   All browser sources (group):
 *     NudgeAll     { deltaDb }  -> { count }
 *     ToggleMuteAll             -> { count }
 *     GetBrowsers               -> { count, browsers: [{ name, db, muted }] }
 *
 * Emitted event: ControlledBrowserChanged { sourceName, db }
 * Registration MUST happen from obs_module_post_load().
 */
namespace VendorModule {

void register_vendor(ActiveBrowser *ctl);
void emit_changed(const char *source_name, float db);

} // namespace VendorModule
