/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <mutex>

/*
 * PluginConfig persists two settings to
 *   <obs config>/plugins/active-source-volume/config.json
 *
 *   nudge_step_db : how many dB one hotkey press moves the active browser
 *                   (default 5.0).
 *   carry_level   : whether the controlled level carries across scene changes
 *                   (default true). See active-browser.hpp.
 *
 * No per-scene mapping is needed any more: the active browser source is
 * detected automatically from the live scene.
 */
class PluginConfig {
public:
	void load();
	void save() const;

	float nudge_step_db() const;
	void set_nudge_step_db(float step);

	bool carry_level() const;
	void set_carry_level(bool carry);

private:
	mutable std::mutex mutex_;
	float nudge_step_db_ = 5.0f;
	bool carry_level_ = true;
};
