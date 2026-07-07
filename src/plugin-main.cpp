/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later

Relative (DCA-style) volume hotkeys for ONE browser source at a time - the
top-most browser on the live scene, or a pinned override - so a streamer's
alert / donation / chat browsers are never touched.
*/
#include "plugin-support.hpp"
#include "config.hpp"
#include "active-browser.hpp"
#include "browser-dca.hpp"
#include "scene-tracker.hpp"
#include "status-dock.hpp"
#include "vendor-request.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <string>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Active Source Volume";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Relative volume hotkeys for the selected browser source on the live scene.";
}

// Process-lifetime singletons (see history: not deleted at unload; removing the
// frontend callback + hotkeys is what makes unload safe, and the dock widget is
// owned by the OBS frontend).
static PluginConfig *g_config = nullptr;
static ActiveBrowser *g_ctl = nullptr;
static SceneTracker *g_tracker = nullptr;

static obs_hotkey_id g_hk_up = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_hk_down = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_hk_mute = OBS_INVALID_HOTKEY_ID;

static void nudge(float delta)
{
	if (!g_config)
		return;
	if (g_config->dca_mode())
		browser_dca::nudge_db(delta);
	else if (g_ctl)
		g_ctl->nudge_db(delta);
}

static void hotkey_vol_up(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed && g_config)
		nudge(g_config->nudge_step_db());
}

static void hotkey_vol_down(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed && g_config)
		nudge(-g_config->nudge_step_db());
}

static void hotkey_mute_toggle(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed || !g_config)
		return;
	if (g_config->dca_mode())
		browser_dca::toggle_mute();
	else if (g_ctl)
		g_ctl->toggle_mute();
}

static void on_frontend_event(enum obs_frontend_event event, void *)
{
	if (g_tracker)
		g_tracker->handle_event(event);
}

bool obs_module_load(void)
{
	plog(LOG_INFO, "Loading version %s", PLUGIN_VERSION);

	g_config = new PluginConfig();
	g_config->load();

	g_ctl = new ActiveBrowser();
	g_ctl->set_on_change([](const std::string &name, float db) { VendorModule::emit_changed(name.c_str(), db); });

	g_tracker = new SceneTracker(*g_ctl, *g_config);

	// Keep these registration IDs stable so existing Stream Deck / hotkey
	// bindings keep working across updates.
	g_hk_up = obs_hotkey_register_frontend("active_browser.vol_up", "Browser Volume: +", hotkey_vol_up, nullptr);
	g_hk_down =
		obs_hotkey_register_frontend("active_browser.vol_down", "Browser Volume: -", hotkey_vol_down, nullptr);
	g_hk_mute = obs_hotkey_register_frontend("active_browser.mute_toggle", "Browser Volume: Toggle Mute",
						 hotkey_mute_toggle, nullptr);

	obs_frontend_add_event_callback(on_frontend_event, nullptr);

	auto *dock = new StatusDock(*g_config, *g_tracker, *g_ctl);
	obs_frontend_add_dock_by_id(DOCK_ID, "Active Source Volume", dock);

	return true;
}

void obs_module_post_load(void)
{
	VendorModule::register_vendor(g_ctl);
	if (g_tracker)
		g_tracker->resolve_current();
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(on_frontend_event, nullptr);

	if (g_hk_up != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(g_hk_up);
	if (g_hk_down != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(g_hk_down);
	if (g_hk_mute != OBS_INVALID_HOTKEY_ID)
		obs_hotkey_unregister(g_hk_mute);

	if (g_ctl)
		g_ctl->shutdown();

	plog(LOG_INFO, "Unloaded");
}
