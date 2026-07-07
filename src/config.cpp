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
	nudge_step_db_ = (float)obs_data_get_double(data, "nudge_step_db");

	const char *ov = obs_data_get_string(data, "override_source");
	override_source_ = ov ? ov : "";

	obs_data_set_default_bool(data, "dca_mode", false);
	dca_mode_ = obs_data_get_bool(data, "dca_mode");

	obs_data_release(data);
	plog(LOG_INFO, "Loaded config: step=%.1f dB, override=%s, dca=%s", nudge_step_db_,
	     override_source_.empty() ? "(auto)" : override_source_.c_str(), dca_mode_ ? "on" : "off");
}

void PluginConfig::save() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	obs_data_t *data = obs_data_create();
	obs_data_set_double(data, "nudge_step_db", nudge_step_db_);
	obs_data_set_string(data, "override_source", override_source_.c_str());
	obs_data_set_bool(data, "dca_mode", dca_mode_);

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

std::string PluginConfig::override_source() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return override_source_;
}

void PluginConfig::set_override_source(const std::string &name)
{
	std::lock_guard<std::mutex> lock(mutex_);
	override_source_ = name;
}

bool PluginConfig::dca_mode() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return dca_mode_;
}

void PluginConfig::set_dca_mode(bool enabled)
{
	std::lock_guard<std::mutex> lock(mutex_);
	dca_mode_ = enabled;
}
