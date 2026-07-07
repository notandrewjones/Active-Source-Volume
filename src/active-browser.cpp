/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "active-browser.hpp"
#include "plugin-support.hpp"

#include <algorithm>
#include <cmath>

float ActiveBrowserController::read_db(obs_source_t *src)
{
	float mul = obs_source_get_volume(src);
	float db = obs_mul_to_db(mul); // 0.0 mul -> -inf
	if (!std::isfinite(db) || db < kFloorDb)
		return kFloorDb;
	return db;
}

void ActiveBrowserController::apply_db(obs_source_t *src, float db)
{
	db = std::max(kFloorDb, std::min(kCeilDb, db));
	if (db <= kFloorDb)
		obs_source_set_volume(src, 0.0f); // hard silence
	else
		obs_source_set_volume(src, obs_db_to_mul(db));
}

void ActiveBrowserController::init(bool carry_level)
{
	std::lock_guard<std::mutex> lock(mutex_);
	carry_level_ = carry_level;
}

void ActiveBrowserController::shutdown()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (weak_) {
		obs_weak_source_release(weak_);
		weak_ = nullptr;
	}
	current_name_.clear();
	have_target_ = false;
}

obs_source_t *ActiveBrowserController::get_strong_locked() const
{
	if (!weak_)
		return nullptr;
	return obs_weak_source_get_source(weak_);
}

void ActiveBrowserController::set_active_source(obs_source_t *source)
{
	std::string new_name;
	float report_db = kFloorDb;
	bool changed = false;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		const char *name = source ? obs_source_get_name(source) : "";
		new_name = name ? name : "";

		if (new_name == current_name_)
			return; // same browser, nothing to do

		if (weak_) {
			obs_weak_source_release(weak_);
			weak_ = nullptr;
		}

		if (source) {
			weak_ = obs_source_get_weak_source(source);

			if (carry_level_ && have_target_) {
				// Write the master level NOW (transition start) so
				// the incoming browser fades in at the right level.
				apply_db(source, target_db_);
			} else {
				// Adopt this source's current level as the master.
				target_db_ = read_db(source);
				have_target_ = true;
			}
			report_db = target_db_;
		}

		current_name_ = new_name;
		changed = true;
	}

	if (changed) {
		plog(LOG_INFO, "Active browser: %s (%.1f dB)", new_name.empty() ? "(none)" : new_name.c_str(),
		     report_db);
		if (on_change_)
			on_change_(new_name, report_db);
	}
}

bool ActiveBrowserController::nudge_db(float delta_db)
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	float cur = read_db(src);
	float next = std::max(kFloorDb, std::min(kCeilDb, cur + delta_db));
	apply_db(src, next);
	target_db_ = next; // keep the master in sync
	have_target_ = true;

	obs_source_release(src);
	return true;
}

bool ActiveBrowserController::set_db(float db)
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	apply_db(src, db);
	target_db_ = std::max(kFloorDb, std::min(kCeilDb, db));
	have_target_ = true;

	obs_source_release(src);
	return true;
}

bool ActiveBrowserController::get_db(float &out) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	out = read_db(src);

	obs_source_release(src);
	return true;
}

bool ActiveBrowserController::toggle_mute()
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	obs_source_set_muted(src, !obs_source_muted(src));

	obs_source_release(src);
	return true;
}

bool ActiveBrowserController::set_mute(bool muted)
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	obs_source_set_muted(src, muted);

	obs_source_release(src);
	return true;
}

bool ActiveBrowserController::get_mute(bool &out) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return false;

	out = obs_source_muted(src);

	obs_source_release(src);
	return true;
}

std::string ActiveBrowserController::active_name() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	if (!src)
		return std::string();

	const char *name = obs_source_get_name(src);
	std::string result = name ? name : "";

	obs_source_release(src);
	return result;
}

bool ActiveBrowserController::has_active() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = get_strong_locked();
	bool r = src != nullptr;
	if (src)
		obs_source_release(src);
	return r;
}

void ActiveBrowserController::set_carry_level(bool carry)
{
	std::lock_guard<std::mutex> lock(mutex_);
	carry_level_ = carry;
}

bool ActiveBrowserController::carry_level() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return carry_level_;
}

void ActiveBrowserController::set_on_change(std::function<void(const std::string &, float)> cb)
{
	std::lock_guard<std::mutex> lock(mutex_);
	on_change_ = std::move(cb);
}
