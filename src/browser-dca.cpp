/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "browser-dca.hpp"
#include "plugin-support.hpp"

#include <obs.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

bool is_browser(obs_source_t *s)
{
	if (!s)
		return false;
	const char *id = obs_source_get_unversioned_id(s);
	return id && strcmp(id, "browser_source") == 0;
}

float clamp_db(float db)
{
	return std::max(browser_dca::kFloorDb, std::min(browser_dca::kCeilDb, db));
}

float read_db(obs_source_t *s)
{
	float db = obs_mul_to_db(obs_source_get_volume(s));
	if (!std::isfinite(db) || db < browser_dca::kFloorDb)
		return browser_dca::kFloorDb;
	return db;
}

void apply_db(obs_source_t *s, float db)
{
	db = clamp_db(db);
	obs_source_set_volume(s, db <= browser_dca::kFloorDb ? 0.0f : obs_db_to_mul(db));
}

} // namespace

int browser_dca::nudge_db(float delta_db)
{
	struct Ctx {
		float delta;
		int count;
	} ctx{delta_db, 0};

	obs_enum_sources(
		[](void *param, obs_source_t *s) -> bool {
			auto *c = static_cast<Ctx *>(param);
			if (is_browser(s)) {
				apply_db(s, read_db(s) + c->delta);
				c->count++;
			}
			return true;
		},
		&ctx);

	if (ctx.count)
		plog(LOG_INFO, "DCA nudge %+.1f dB applied to %d browser source(s)", delta_db, ctx.count);
	return ctx.count;
}

int browser_dca::toggle_mute()
{
	struct Query {
		bool any_unmuted;
	} query{false};

	obs_enum_sources(
		[](void *param, obs_source_t *s) -> bool {
			auto *q = static_cast<Query *>(param);
			if (is_browser(s) && !obs_source_muted(s))
				q->any_unmuted = true;
			return true;
		},
		&query);

	struct Apply {
		bool target;
		int count;
	} apply{query.any_unmuted, 0};

	obs_enum_sources(
		[](void *param, obs_source_t *s) -> bool {
			auto *a = static_cast<Apply *>(param);
			if (is_browser(s)) {
				obs_source_set_muted(s, a->target);
				a->count++;
			}
			return true;
		},
		&apply);

	if (apply.count)
		plog(LOG_INFO, "DCA %s %d browser source(s)", apply.target ? "muted" : "unmuted", apply.count);
	return apply.count;
}

std::vector<browser_dca::Entry> browser_dca::snapshot()
{
	std::vector<Entry> out;

	obs_enum_sources(
		[](void *param, obs_source_t *s) -> bool {
			auto *v = static_cast<std::vector<Entry> *>(param);
			if (is_browser(s)) {
				const char *name = obs_source_get_name(s);
				v->push_back({name ? name : "", read_db(s), obs_source_muted(s)});
			}
			return true;
		},
		&out);

	std::sort(out.begin(), out.end(), [](const Entry &a, const Entry &b) { return a.name < b.name; });
	return out;
}
