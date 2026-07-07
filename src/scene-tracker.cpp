/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "scene-tracker.hpp"
#include "active-browser.hpp"
#include "plugin-support.hpp"

#include <obs.h>

#include <cstring>

namespace {

bool is_browser_source(obs_source_t *src)
{
	if (!src)
		return false;
	const char *id = obs_source_get_unversioned_id(src);
	return id && strcmp(id, "browser_source") == 0;
}

// Walks the program scene's items looking for the top-most visible browser
// source. Descends into groups (still the same scene), but deliberately does
// NOT descend into nested scenes - those are "other stuff" we ignore.
struct ResolveCtx {
	obs_source_t *found = nullptr; // borrowed
};

bool enum_item_cb(obs_scene_t * /*scene*/, obs_sceneitem_t *item, void *param)
{
	auto *ctx = static_cast<ResolveCtx *>(param);

	if (!obs_sceneitem_visible(item))
		return true; // hidden -> skip (and don't descend hidden groups)

	if (obs_sceneitem_is_group(item)) {
		// Same callback recurses through the group's children.
		obs_sceneitem_group_enum_items(item, enum_item_cb, param);
		return true;
	}

	obs_source_t *src = obs_sceneitem_get_source(item);
	if (is_browser_source(src))
		ctx->found = src; // enumeration is bottom->top, so last == top-most

	return true; // keep going; ignore nested scenes / mics / everything else
}

} // namespace

SceneTracker::SceneTracker(ActiveBrowserController &ctl) : ctl_(ctl) {}

void SceneTracker::handle_event(enum obs_frontend_event event)
{
	switch (event) {
	// Fires at transition START; obs_frontend_get_current_scene() already
	// returns the incoming scene here, which is exactly when we want to
	// (re)resolve and, if carrying a level, write it before the fade-in.
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
		resolve_current();
		break;
	default:
		break;
	}
}

void SceneTracker::resolve_current()
{
	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source) {
		ctl_.set_active_source(nullptr);
		return;
	}

	obs_scene_t *scene = obs_scene_from_source(scene_source);
	ResolveCtx ctx;
	if (scene)
		obs_scene_enum_items(scene, enum_item_cb, &ctx);

	ctl_.set_active_source(ctx.found); // nullptr -> no browser in this scene

	obs_source_release(scene_source);
}
