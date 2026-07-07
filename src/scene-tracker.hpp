/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <obs-frontend-api.h>

class ActiveBrowser;
class PluginConfig;

/*
 * SceneTracker decides WHICH single browser source is controlled and hands it
 * to ActiveBrowser:
 *
 *   1. If the user set an override (a specific browser source name), that one is
 *      always controlled, regardless of scene.
 *   2. Otherwise, the top-most VISIBLE browser source in the live program scene
 *      (groups are searched; nested scenes are not).
 *
 * It re-resolves on scene changes so, in auto mode, control follows whatever
 * browser is on screen. Selecting a source never changes any volume.
 */
class SceneTracker {
public:
	SceneTracker(ActiveBrowser &ctl, PluginConfig &cfg);

	void handle_event(enum obs_frontend_event event);
	void resolve_current();

private:
	ActiveBrowser &ctl_;
	PluginConfig &cfg_;
};
