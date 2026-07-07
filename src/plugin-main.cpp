/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later

One Stream Deck button (or dial) that controls the volume of the browser source
on the scene currently live on OBS's program output.
*/
#include "plugin-support.hpp"
#include "config.hpp"
#include "active-browser.hpp"
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
	return "Control the volume of the browser source on the live scene.";
}

/*
 * Process-lifetime singletons (intentionally never deleted in unload; see the
 * note in the v1 history - unregistering the callback + hotkeys is what makes
 * unload safe, and the dock widget is owned by the OBS frontend).
 */
static PluginConfig *g_config = nullptr;
static ActiveBrowserController *g_ctl = nullptr;
static SceneTracker *g_tracker = nullptr;

static obs_hotkey_id g_hk_up = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_hk_down = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_hk_mute = OBS_INVALID_HOTKEY_ID;

// ---- hotkey callbacks -------------------------------------------------------

static void hotkey_vol_up(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed && g_ctl && g_config)
		g_ctl->nudge_db(g_config->nudge_step_db());
}

static void hotkey_vol_down(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed && g_ctl && g_config)
		g_ctl->nudge_db(-g_config->nudge_step_db());
}

static void hotkey_mute_toggle(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed && g_ctl)
		g_ctl->toggle_mute();
}

// ---- single frontend event callback ----------------------------------------

static void on_frontend_event(enum obs_frontend_event event, void *)
{
	if (g_tracker)
		g_tracker->handle_event(event);
}

// ---- module lifecycle -------------------------------------------------------

bool obs_module_load(void)
{
	plog(LOG_INFO, "Loading version %s", PLUGIN_VERSION);

	g_config = new PluginConfig();
	g_config->load();

	g_ctl = new ActiveBrowserController();
	g_ctl->init(g_config->carry_level());
	g_ctl->set_on_change(
		[](const std::string &name, float db) { VendorModule::emit_active_changed(name.c_str(), db); });

	g_tracker = new SceneTracker(*g_ctl);

	// Frontend hotkeys are automatically saved/restored by OBS.
	g_hk_up = obs_hotkey_register_frontend("active_browser.vol_up", "Active Browser: Volume +", hotkey_vol_up,
					       nullptr);
	g_hk_down = obs_hotkey_register_frontend("active_browser.vol_down", "Active Browser: Volume -", hotkey_vol_down,
						 nullptr);
	g_hk_mute = obs_hotkey_register_frontend("active_browser.mute_toggle", "Active Browser: Toggle Mute",
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
