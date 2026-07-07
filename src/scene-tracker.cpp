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
#include <set>
#include <string>
#include <vector>

namespace {

bool is_browser(obs_source_t *src)
{
	if (!src)
		return false;
	const char *id = obs_source_get_unversioned_id(src);
	return id && strcmp(id, "browser_source") == 0;
}

// Collects the names of visible browser sources in a scene, in z-order
// (bottom-to-top). Descends into groups (same scene) but not nested scenes.
bool enum_item_cb(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	auto *names = static_cast<std::vector<std::string> *>(param);

	if (!obs_sceneitem_visible(item))
		return true;

	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, enum_item_cb, param);
		return true;
	}

	obs_source_t *src = obs_sceneitem_get_source(item);
	if (is_browser(src)) {
		const char *n = obs_source_get_name(src);
		if (n && *n)
			names->emplace_back(n);
	}
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
	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source) {
		ctl_.set_source(nullptr);
		return;
	}

	// Visible browser sources on the live scene, bottom-to-top.
	std::vector<std::string> visible;
	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (scene)
		obs_scene_enum_items(scene, enum_item_cb, &visible);

	std::set<std::string> selected = cfg_.selected_sources();

	std::string chosen;
	if (selected.empty()) {
		// Auto: top-most visible browser.
		if (!visible.empty())
			chosen = visible.back();
	} else {
		// Top-most visible browser that the user checked.
		for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
			if (selected.count(*it)) {
				chosen = *it;
				break;
			}
		}
	}

	if (chosen.empty()) {
		ctl_.set_source(nullptr);
	} else {
		obs_source_t *src = obs_get_source_by_name(chosen.c_str());
		ctl_.set_source(src);
		if (src)
			obs_source_release(src);
	}

	obs_source_release(scene_source);
}
