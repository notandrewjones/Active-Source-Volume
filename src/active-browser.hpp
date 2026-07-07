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
 * ActiveBrowser holds the single browser source currently being controlled and
 * applies RELATIVE volume changes to it (add N dB to its current level). It only
 * ever touches this one source, so a streamer's other browser sources - alerts,
 * donations, chat - are never affected.
 *
 * Which source this is gets decided by SceneTracker (an explicit override, or
 * the top-most visible browser in the live scene). The source is held as a weak
 * reference so we never keep it alive past its natural lifetime.
 */
class ActiveBrowser {
public:
	static constexpr float kFloorDb = -96.0f;
	static constexpr float kCeilDb = 26.0f;

	// Point at the controlled source (borrowed ref; stored weak). nullptr clears.
	// Does not change any volume - selecting a source never moves audio.
	void set_source(obs_source_t *source);

	// Relative dB trim of the controlled source. Returns true if one was set.
	bool nudge_db(float delta_db);
	bool toggle_mute();

	bool get_db(float &out) const;
	bool get_mute(bool &out) const;
	std::string name() const;
	bool has() const;

	void shutdown();

	// Fired (outside the lock) when the controlled source changes: (name, db).
	void set_on_change(std::function<void(const std::string &, float)> cb);

private:
	obs_source_t *strong_locked() const;
	static float read_db(obs_source_t *src);
	static void apply_db(obs_source_t *src, float db);

	mutable std::mutex mutex_;
	obs_weak_source_t *weak_ = nullptr;
	std::string name_;
	std::function<void(const std::string &, float)> on_change_;
};
