/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "scene-tracker.hpp"
#include "active-browser.hpp"
#include "config.hpp"
#include "plugin-support.hpp"

#include <obs.h>

#include <cstring>
#include <string>

namespace {

bool is_browser(obs_source_t *src)
{
	if (!src)
		return false;
	const char *id = obs_source_get_unversioned_id(src);
	return id && strcmp(id, "browser_source") == 0;
}

// Finds the top-most visible browser source in a scene. Descends into groups
// (same scene) but deliberately not into nested scenes.
struct ResolveCtx {
	obs_source_t *found; // borrowed
};

bool enum_item_cb(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	auto *ctx = static_cast<ResolveCtx *>(param);

	if (!obs_sceneitem_visible(item))
		return true;

	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, enum_item_cb, param);
		return true;
	}

	obs_source_t *src = obs_sceneitem_get_source(item);
	if (is_browser(src))
		ctx->found = src; // bottom-to-top enumeration -> last wins == top-most

	return true;
}

} // namespace

SceneTracker::SceneTracker(ActiveBrowser &ctl, PluginConfig &cfg) : ctl_(ctl), cfg_(cfg) {}

void SceneTracker::handle_event(enum obs_frontend_event event)
{
	switch (event) {
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
	// 1. Explicit override wins, regardless of scene.
	std::string override_name = cfg_.override_source();
	if (!override_name.empty()) {
		obs_source_t *src = obs_get_source_by_name(override_name.c_str());
		if (src && is_browser(src)) {
			ctl_.set_source(src);
			obs_source_release(src);
			return;
		}
		if (src)
			obs_source_release(src);
		plog(LOG_WARNING, "Override browser \"%s\" not found; using top-most in live scene",
		     override_name.c_str());
	}

	// 2. Auto: top-most visible browser in the live program scene.
	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source) {
		ctl_.set_source(nullptr);
		return;
	}

	obs_scene_t *scene = obs_scene_from_source(scene_source);
	ResolveCtx ctx{nullptr};
	if (scene)
		obs_scene_enum_items(scene, enum_item_cb, &ctx);

	ctl_.set_source(ctx.found);
	obs_source_release(scene_source);
}
