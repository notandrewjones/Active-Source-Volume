/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "vendor-request.hpp"
#include "active-browser.hpp"
#include "browser-dca.hpp"
#include "plugin-support.hpp"

#include <obs.h>
#include <obs-websocket-api.h>

#include <string>

namespace {

obs_websocket_vendor g_vendor = nullptr;
ActiveBrowser *g_ctl = nullptr;

void write_state(obs_data_t *response)
{
	std::string name = g_ctl ? g_ctl->name() : "";
	obs_data_set_string(response, "sourceName", name.c_str());

	float db = 0.0f;
	bool muted = false;
	bool has = g_ctl && g_ctl->get_db(db);
	obs_data_set_bool(response, "hasSource", has);
	if (has) {
		g_ctl->get_mute(muted);
		obs_data_set_double(response, "db", db);
		obs_data_set_bool(response, "muted", muted);
	}
}

void req_nudge(obs_data_t *request, obs_data_t *response, void *)
{
	double delta = obs_data_get_double(request, "deltaDb");
	if (g_ctl)
		g_ctl->nudge_db((float)delta);
	write_state(response);
}

void req_toggle_mute(obs_data_t *, obs_data_t *response, void *)
{
	if (g_ctl)
		g_ctl->toggle_mute();
	write_state(response);
}

void req_get(obs_data_t *, obs_data_t *response, void *)
{
	write_state(response);
}

void req_nudge_all(obs_data_t *request, obs_data_t *response, void *)
{
	double delta = obs_data_get_double(request, "deltaDb");
	int count = browser_dca::nudge_db((float)delta);
	obs_data_set_int(response, "count", count);
}

void req_toggle_mute_all(obs_data_t *, obs_data_t *response, void *)
{
	int count = browser_dca::toggle_mute();
	obs_data_set_int(response, "count", count);
}

void req_get_browsers(obs_data_t *, obs_data_t *response, void *)
{
	auto entries = browser_dca::snapshot();
	obs_data_array_t *arr = obs_data_array_create();
	for (const auto &e : entries) {
		obs_data_t *item = obs_data_create();
		obs_data_set_string(item, "name", e.name.c_str());
		obs_data_set_double(item, "db", e.db);
		obs_data_set_bool(item, "muted", e.muted);
		obs_data_array_push_back(arr, item);
		obs_data_release(item);
	}
	obs_data_set_int(response, "count", (long long)entries.size());
	obs_data_set_array(response, "browsers", arr);
	obs_data_array_release(arr);
}

} // namespace

namespace VendorModule {

void register_vendor(ActiveBrowser *ctl)
{
	g_ctl = ctl;

	g_vendor = obs_websocket_register_vendor(VENDOR_NAME);
	if (!g_vendor) {
		plog(LOG_INFO, "obs-websocket not present; vendor requests unavailable (hotkeys still work)");
		return;
	}

	obs_websocket_vendor_register_request(g_vendor, "NudgeVolume", req_nudge, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "ToggleMute", req_toggle_mute, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "GetControlled", req_get, nullptr);

	// Group (all browser sources) - useful regardless of the dock DCA toggle.
	obs_websocket_vendor_register_request(g_vendor, "NudgeAll", req_nudge_all, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "ToggleMuteAll", req_toggle_mute_all, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "GetBrowsers", req_get_browsers, nullptr);

	plog(LOG_INFO, "Registered obs-websocket vendor \"%s\"", VENDOR_NAME);
}

void emit_changed(const char *source_name, float db)
{
	if (!g_vendor)
		return;
	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "sourceName", source_name ? source_name : "");
	obs_data_set_double(data, "db", db);
	obs_websocket_vendor_emit_event(g_vendor, "ControlledBrowserChanged", data);
	obs_data_release(data);
}

} // namespace VendorModule
