/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "active-browser.hpp"
#include "plugin-support.hpp"

#include <algorithm>
#include <cmath>

float ActiveBrowser::read_db(obs_source_t *src)
{
	float db = obs_mul_to_db(obs_source_get_volume(src));
	if (!std::isfinite(db) || db < kFloorDb)
		return kFloorDb;
	return db;
}

void ActiveBrowser::apply_db(obs_source_t *src, float db)
{
	db = std::max(kFloorDb, std::min(kCeilDb, db));
	obs_source_set_volume(src, db <= kFloorDb ? 0.0f : obs_db_to_mul(db));
}

obs_source_t *ActiveBrowser::strong_locked() const
{
	return weak_ ? obs_weak_source_get_source(weak_) : nullptr;
}

void ActiveBrowser::set_source(obs_source_t *source)
{
	std::string new_name;
	float report_db = kFloorDb;
	bool changed = false;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		const char *name = source ? obs_source_get_name(source) : "";
		new_name = name ? name : "";
		if (new_name == name_)
			return;

		if (weak_) {
			obs_weak_source_release(weak_);
			weak_ = nullptr;
		}
		if (source) {
			weak_ = obs_source_get_weak_source(source);
			report_db = read_db(source);
		}
		name_ = new_name;
		changed = true;
	}

	if (changed) {
		plog(LOG_INFO, "Controlling browser: %s", new_name.empty() ? "(none)" : new_name.c_str());
		if (on_change_)
			on_change_(new_name, report_db);
	}
}

bool ActiveBrowser::nudge_db(float delta_db)
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	if (!src)
		return false;

	apply_db(src, read_db(src) + delta_db);

	obs_source_release(src);
	return true;
}

bool ActiveBrowser::toggle_mute()
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	if (!src)
		return false;

	obs_source_set_muted(src, !obs_source_muted(src));

	obs_source_release(src);
	return true;
}

bool ActiveBrowser::get_db(float &out) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	if (!src)
		return false;
	out = read_db(src);
	obs_source_release(src);
	return true;
}

bool ActiveBrowser::get_mute(bool &out) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	if (!src)
		return false;
	out = obs_source_muted(src);
	obs_source_release(src);
	return true;
}

std::string ActiveBrowser::name() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	if (!src)
		return std::string();
	const char *n = obs_source_get_name(src);
	std::string result = n ? n : "";
	obs_source_release(src);
	return result;
}

bool ActiveBrowser::has() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	obs_source_t *src = strong_locked();
	bool r = src != nullptr;
	if (src)
		obs_source_release(src);
	return r;
}

void ActiveBrowser::shutdown()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (weak_) {
		obs_weak_source_release(weak_);
		weak_ = nullptr;
	}
	name_.clear();
}

void ActiveBrowser::set_on_change(std::function<void(const std::string &, float)> cb)
{
	std::lock_guard<std::mutex> lock(mutex_);
	on_change_ = std::move(cb);
}
