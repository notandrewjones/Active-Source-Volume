/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "vendor-request.hpp"
#include "active-browser.hpp"
#include "plugin-support.hpp"

#include <obs.h>
#include <obs-websocket-api.h>

#include <string>

namespace {

obs_websocket_vendor g_vendor = nullptr;
ActiveBrowserController *g_ctl = nullptr;

void write_state(obs_data_t *response)
{
	std::string name = g_ctl ? g_ctl->active_name() : "";
	obs_data_set_string(response, "sourceName", name.c_str());

	float db = 0.0f;
	bool muted = false;
	bool has = g_ctl && g_ctl->get_db(db);
	obs_data_set_bool(response, "hasActiveBrowser", has);
	if (has) {
		g_ctl->get_mute(muted);
		obs_data_set_double(response, "db", db);
		obs_data_set_bool(response, "muted", muted);
	}
}

void req_get(obs_data_t *, obs_data_t *response, void *)
{
	write_state(response);
}

void req_nudge(obs_data_t *request, obs_data_t *response, void *)
{
	double delta = obs_data_get_double(request, "deltaDb");
	if (g_ctl)
		g_ctl->nudge_db((float)delta);
	write_state(response);
}

void req_set_volume(obs_data_t *request, obs_data_t *response, void *)
{
	double db = obs_data_get_double(request, "db");
	if (g_ctl)
		g_ctl->set_db((float)db);
	write_state(response);
}

void req_set_mute(obs_data_t *request, obs_data_t *response, void *)
{
	bool muted = obs_data_get_bool(request, "muted");
	if (g_ctl)
		g_ctl->set_mute(muted);
	write_state(response);
}

void req_toggle_mute(obs_data_t *, obs_data_t *response, void *)
{
	if (g_ctl)
		g_ctl->toggle_mute();
	write_state(response);
}

} // namespace

namespace VendorModule {

void register_vendor(ActiveBrowserController *ctl)
{
	g_ctl = ctl;

	g_vendor = obs_websocket_register_vendor(VENDOR_NAME);
	if (!g_vendor) {
		plog(LOG_INFO,
		     "obs-websocket not present; dial/absolute requests unavailable (hotkeys still work)");
		return;
	}

	obs_websocket_vendor_register_request(g_vendor, "GetActiveBrowser", req_get, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "NudgeVolume", req_nudge, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "SetVolume", req_set_volume, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "SetMute", req_set_mute, nullptr);
	obs_websocket_vendor_register_request(g_vendor, "ToggleMute", req_toggle_mute, nullptr);

	plog(LOG_INFO, "Registered obs-websocket vendor \"%s\"", VENDOR_NAME);
}

void emit_active_changed(const char *source_name, float db)
{
	if (!g_vendor)
		return;
	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "sourceName", source_name ? source_name : "");
	obs_data_set_double(data, "db", db);
	obs_websocket_vendor_emit_event(g_vendor, "ActiveBrowserChanged", data);
	obs_data_release(data);
}

} // namespace VendorModule
