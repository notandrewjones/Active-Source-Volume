/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <mutex>
#include <string>

/*
 * PluginConfig persists:
 *   nudge_step_db   : dB shift per hotkey press (default 5.0).
 *   override_source : name of a specific browser source to always control;
 *                     empty means auto (top-most browser in the live scene).
 *   dca_mode        : when true, hotkeys act on ALL browser sources at once.
 * Stored at <obs config>/plugins/active-source-volume/config.json
 */
class PluginConfig {
public:
	void load();
	void save() const;

	float nudge_step_db() const;
	void set_nudge_step_db(float step);

	std::string override_source() const;
	void set_override_source(const std::string &name);

	bool dca_mode() const;
	void set_dca_mode(bool enabled);

private:
	mutable std::mutex mutex_;
	float nudge_step_db_ = 5.0f;
	std::string override_source_;
	bool dca_mode_ = false;
};
