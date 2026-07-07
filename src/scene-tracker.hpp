/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <obs-frontend-api.h>

class ActiveBrowserController;

/*
 * SceneTracker watches the OBS program scene and, on every change, finds the
 * active browser source in it and hands it to the ActiveBrowserController.
 *
 * "Active browser source" = the top-most VISIBLE source of type
 * "browser_source" among the program scene's items (descending into groups but
 * NOT into nested scenes). Everything else - mics, media, nested scenes, images
 * - is ignored by design.
 *
 * Resolution runs at transition START (OBS_FRONTEND_EVENT_SCENE_CHANGED), which
 * is what keeps audio artifact-free through transitions (see active-browser.hpp).
 */
class SceneTracker {
public:
	explicit SceneTracker(ActiveBrowserController &ctl);

	void handle_event(enum obs_frontend_event event);
	void resolve_current();

private:
	ActiveBrowserController &ctl_;
};
