/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <obs.h>

#include <functional>
#include <mutex>
#include <string>

/*
 * ActiveBrowserController owns "the currently active browser source" and all
 * volume/mute operations against it. Volume is worked in decibels via OBS's
 * mul<->dB helpers, so the hotkeys map directly to +/- dB nudges.
 *
 * Transition safety
 * -----------------
 * OBS applies a scene transition's audio crossfade as a multiplier ON TOP OF
 * each source's base volume; the transition never rewrites the source volume
 * itself. So the rule that avoids the "audio jumps during the transition then
 * drops after it" artifact is simple: only ever change a source's base volume
 * at the START of a transition (when OBS_FRONTEND_EVENT_SCENE_CHANGED fires),
 * never at the end. set_active_source() is therefore called at transition start
 * and, when carry-level is on, writes the target dB immediately so the incoming
 * browser fades in already at the right level. We never touch volume on
 * transition-stop.
 *
 * Carry level across scenes
 * -------------------------
 * With carry_level enabled (default) the controller keeps a single "master" dB
 * and applies it to whichever browser becomes active, so the active-browser
 * channel behaves like one continuous fader across scenes. Disable it to let
 * each browser keep its own independent level (still artifact-free, since we
 * simply don't write on switch in that mode).
 */
class ActiveBrowserController {
public:
	static constexpr float kFloorDb = -96.0f; // at/below this we hard-silence
	static constexpr float kCeilDb = 26.0f;

	void init(bool carry_level);
	void shutdown();

	// Point at the active browser source (borrowed ref; stored as weak).
	// nullptr clears. See "Transition safety" above for write behaviour.
	void set_active_source(obs_source_t *source);

	// Volume ops (dB). Return true if there was a live active browser.
	bool nudge_db(float delta_db);
	bool set_db(float db);
	bool get_db(float &out) const;

	bool toggle_mute();
	bool set_mute(bool muted);
	bool get_mute(bool &out) const;

	std::string active_name() const;
	bool has_active() const;

	void set_carry_level(bool carry);
	bool carry_level() const;

	// Fired (outside the lock) when the active source changes:
	// (source name or "" , current dB).
	void set_on_change(std::function<void(const std::string &, float)> cb);

private:
	obs_source_t *get_strong_locked() const;
	static float read_db(obs_source_t *src);            // floored, never -inf
	static void apply_db(obs_source_t *src, float db);  // writes base volume

	mutable std::mutex mutex_;
	obs_weak_source_t *weak_ = nullptr;
	std::string current_name_;

	bool have_target_ = false;
	float target_db_ = 0.0f;
	bool carry_level_ = true;

	std::function<void(const std::string &, float)> on_change_;
};
