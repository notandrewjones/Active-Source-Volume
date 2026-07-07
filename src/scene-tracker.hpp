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
 * to ActiveBrowser, re-resolving on scene changes:
 *
 *   - If the user has checked one or more browser sources, the controlled source
 *     is the top-most CHECKED, visible browser on the live scene. (If none of
 *     the checked sources are on the live scene, nothing is controlled.) This
 *     lets you hand-pick your content browser per scene even when it's buried
 *     low in the source list.
 *   - If nothing is checked, it falls back to the top-most visible browser on
 *     the live scene (auto).
 *
 * Groups are searched; nested scenes are not. Selecting never changes volume.
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
