/*
Active Source Volume - an OBS Studio plugin
SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "config.hpp"
#include "plugin-support.hpp"

#include <obs-module.h>
#include <util/platform.h>

#include <string>

namespace {

std::string config_file_path()
{
	char *path = obs_module_config_path("config.json");
	std::string result(path ? path : "");
	bfree(path);

	auto slash = result.find_last_of('/');
	if (slash != std::string::npos)
		os_mkdirs(result.substr(0, slash).c_str());
	return result;
}

} // namespace

void PluginConfig::load()
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::string path = config_file_path();
	obs_data_t *data = obs_data_create_from_json_file(path.c_str());
	if (!data) {
		plog(LOG_INFO, "No existing config at %s, using defaults", path.c_str());
		return;
	}

	obs_data_set_default_double(data, "nudge_step_db", 5.0);
	obs_data_set_default_bool(data, "carry_level", true);

	nudge_step_db_ = (float)obs_data_get_double(data, "nudge_step_db");
	carry_level_ = obs_data_get_bool(data, "carry_level");

	obs_data_release(data);
	plog(LOG_INFO, "Loaded config: step=%.1f dB, carry_level=%s", nudge_step_db_,
	     carry_level_ ? "true" : "false");
}

void PluginConfig::save() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	obs_data_t *data = obs_data_create();
	obs_data_set_double(data, "nudge_step_db", nudge_step_db_);
	obs_data_set_bool(data, "carry_level", carry_level_);

	std::string path = config_file_path();
	if (!obs_data_save_json_safe(data, path.c_str(), "tmp", "bak"))
		plog(LOG_WARNING, "Failed to save config to %s", path.c_str());

	obs_data_release(data);
}

float PluginConfig::nudge_step_db() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return nudge_step_db_;
}

void PluginConfig::set_nudge_step_db(float step)
{
	std::lock_guard<std::mutex> lock(mutex_);
	nudge_step_db_ = step;
}

bool PluginConfig::carry_level() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return carry_level_;
}

void PluginConfig::set_carry_level(bool carry)
{
	std::lock_guard<std::mutex> lock(mutex_);
	carry_level_ = carry;
}
