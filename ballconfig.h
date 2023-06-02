#pragma once

#include "config.h"
#include "logging.h"
#include "natives.h"

#define MOD_CONFIG_FILE "golfgame.cfg"

inline class
{
	config config_file;
	struct
	{
		unsigned long ball_model_hash;
		unsigned long goal_model_hash;
		float ball_launch_strength;
		float ball_respawn_timeout;
		float ball_spawn_search_dist;
		float ball_respawn_notify_time;
		float ball_timeout_time;
		float ball_hit_ped_fling_multiplier;
		float ball_goal_celebration_time;
		int ball_launch_trajectory_sim_time;
		bool enable_help_texts;
		bool peds_are_invincible;
		bool notify_on_manual_respawn;
		bool enable_attempts_counter;
		bool enable_respawn_sound;
		bool enable_out_of_bounds_check;
	} _config;

  public:
	const auto &operator()() const
	{
		return _config;
	}

	void read_config()
	{
		mod_log("(Re-)loading config\n");

		config_file = { MOD_CONFIG_FILE };
#define v(t, n) config_file.get_value<t>(n)
		_config = {
			.ball_model_hash                 = GET_HASH_KEY(v(std::string, "ball_model_name")),
			.goal_model_hash                 = GET_HASH_KEY(v(std::string, "goal_model_name")),
			.ball_launch_strength            = v(float, "ball_launch_strength"),
			.ball_respawn_timeout            = v(float, "ball_respawn_timeout"),
			.ball_spawn_search_dist          = v(float, "ball_spawn_search_dist"),
			.ball_respawn_notify_time        = v(float, "ball_respawn_notify_time"),
			.ball_timeout_time               = v(float, "ball_timeout_time"),
			.ball_hit_ped_fling_multiplier   = v(float, "ball_hit_ped_fling_multiplier"),
			.ball_goal_celebration_time      = v(float, "ball_goal_celebration_time"),
			.ball_launch_trajectory_sim_time = v(int, "ball_launch_trajectory_simulation_time"),
			.enable_help_texts               = v(bool, "enable_hints"),
			.peds_are_invincible             = v(bool, "peds_are_invincible"),
			.notify_on_manual_respawn        = v(bool, "notify_on_manual_respawn"),
			.enable_attempts_counter         = v(bool, "enable_attempts_counter"),
			.enable_respawn_sound            = v(bool, "enable_respawn_sound"),
			.enable_out_of_bounds_check      = v(bool, "enable_out_of_bounds_check"),
		};
#undef v
	}
} ballconfig;