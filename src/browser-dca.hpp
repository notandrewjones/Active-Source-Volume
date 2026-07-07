/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <string>
#include <vector>

/*
 * Optional "DCA mode" group operations: treat every browser source in the scene
 * collection as one DCA/VCA group. A nudge applies the same dB *offset* to each,
 * relative to its own level, preserving their relative balance. Stateless - each
 * call enumerates the live sources.
 *
 * This is only used when the user enables DCA mode in the dock; the default
 * single-source behaviour lives in ActiveBrowser / SceneTracker.
 */
namespace browser_dca {

constexpr float kFloorDb = -96.0f;
constexpr float kCeilDb = 26.0f;

struct Entry {
	std::string name;
	float db; // floored, never -inf
	bool muted;
};

// Relative dB trim of every browser source. Returns count affected.
int nudge_db(float delta_db);

// If any browser source is unmuted, mute all; otherwise unmute all. Returns count.
int toggle_mute();

// State of every browser source, sorted by name (for the dock / websocket).
std::vector<Entry> snapshot();

} // namespace browser_dca
