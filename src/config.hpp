/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <mutex>
#include <set>
#include <string>

/*
 * PluginConfig persists:
 *   nudge_step_db    : dB shift per hotkey press (default 5.0).
 *   selected_sources : names of browser sources the user has checked. When any
 *                      are checked, only those are controlled (and only while
 *                      one is on the live scene). Empty = auto (top-most browser
 *                      on the live scene).
 *   dca_mode         : when true, hotkeys act on ALL browser sources at once.
 * Stored at <obs config>/plugins/active-source-volume/config.json
 */
class PluginConfig {
public:
	void load();
	void save() const;

	float nudge_step_db() const;
	void set_nudge_step_db(float step);

	bool dca_mode() const;
	void set_dca_mode(bool enabled);

	std::set<std::string> selected_sources() const;
	bool is_selected(const std::string &name) const;
	void set_selected(const std::string &name, bool on);
	void clear_selection();
	bool has_selection() const;

private:
	mutable std::mutex mutex_;
	float nudge_step_db_ = 5.0f;
	bool dca_mode_ = false;
	std::set<std::string> selected_;
};
