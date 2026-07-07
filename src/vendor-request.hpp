/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

class ActiveBrowserController;

/*
 * obs-websocket vendor requests for dial / absolute control of the active
 * browser (vendor name: "active-source-volume"):
 *
 *   GetActiveBrowser  -> { sourceName, db, muted, hasActiveBrowser }
 *   NudgeVolume  { deltaDb }  -> state
 *   SetVolume    { db }       -> state
 *   SetMute      { muted }    -> state
 *   ToggleMute                -> state
 *
 * Emitted event:  ActiveBrowserChanged { sourceName, db }
 *
 * Registration MUST happen from obs_module_post_load().
 */
namespace VendorModule {

void register_vendor(ActiveBrowserController *ctl);
void emit_active_changed(const char *source_name, float db);

} // namespace VendorModule
