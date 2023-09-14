#include <stdafx.h>

#include "main.h"

#include "ballconfig.h"
#include "labels.h"

#define BALL_LAUNCH_INDICATOR_LEN 35.f
static_assert(BALL_LAUNCH_INDICATOR_LEN > 0);

static Vector3 ball_spawn_pos  = { 0.f, 0.f, 0.f };
static Vector3 last_launch_rot = { 0.f, 0.f, 0.f };
static Object spawn_ball       = 0;
static Object ball             = 0;
static enum class mod_state
{
	idle,
	launching_ball,
	launching_ball2,
	spectating_ball
} mod_state                              = {};
static Vector3 launch_vec_norm           = { 0.f, 0.f, 0.f };
static int notify_scaleform              = 0;
static float ball_respawn_notify_timer   = 0.f;
static float ball_timeout_timer          = 0.f;
static float ball_goal_celebration_timer = 0.f;
static enum class ball_respawn_reason
{
	out_of_bounds,
	stuck,
	manually,
	hit_target
} ball_respawn_reason            = {};
static Object ball_target        = 0;
static uint16_t ball_cur_attempt = 0;

static bool toggle_mod_state     = false;
static bool toggle_ball_respawn  = false;
static bool toggle_win           = false;

static void delete_ball()
{
	if (ball && DOES_ENTITY_EXIST(ball))
	{
		mod_log("Removing previous ball %i\n", ball);
		DELETE_OBJECT(&ball);
	}
}

static void respawn_ball()
{
	delete_ball();

	mod_log("Creating new ball\n");
	REQUEST_MODEL(ballconfig().ball_model_hash);
	while (!HAS_MODEL_LOADED(ballconfig().ball_model_hash))
	{
		scriptWait(0);
	}

	ball = CREATE_OBJECT(ballconfig().ball_model_hash, ball_spawn_pos.x, ball_spawn_pos.y, ball_spawn_pos.z, true,
	                     false, true);

	SET_ENTITY_ROTATION(ball, last_launch_rot.x, last_launch_rot.y, last_launch_rot.z, 0, false);
	SET_ENTITY_ROTATION(PLAYER_PED_ID(), last_launch_rot.x, last_launch_rot.y, last_launch_rot.z, 0, false);
	invoke<Void>(0x2AED6301F67007D5, ball); // _DISABLE_CAM_COLLISION_FOR_ENTITY

	mod_log("Created new ball %i\n", ball);

	RENDER_SCRIPT_CAMS(false, false, 0, false, false, false);

	int prev_cam_mode = GET_FOLLOW_PED_CAM_VIEW_MODE();
	SET_FOLLOW_PED_CAM_VIEW_MODE(4);
	scriptWait(0);
	SET_FOLLOW_PED_CAM_VIEW_MODE(0);
	SET_FOLLOW_PED_CAM_VIEW_MODE(prev_cam_mode);
	_ANIMATE_GAMEPLAY_CAM_ZOOM(1.f, 10.f);
	ANIMPOSTFX_PLAY("RaceTurbo", 200, false);

	if (ballconfig().enable_respawn_sound)
		PLAY_SOUND_FRONTEND(-1, "CHALLENGE_UNLOCKED", "HUD_AWARDS", false);
}

static void set_ball_spawn_pos(const Vector3 &pos)
{
	mod_log("New ball spawn coords: x: %f y: %f z: %f\n", pos.x, pos.y, pos.z);
	ball_spawn_pos = pos;
}

static void display_attempts_counter()
{
	DRAW_SPRITE("shared", "bggradient_32x1024", .9f, .95f, .115f, .04f, 90.f, 255, 255, 255, 160, false);
	BEGIN_TEXT_COMMAND_DISPLAY_TEXT(HELP_ATTEMPT);
	SET_TEXT_FONT(2);
	SET_TEXT_SCALE(.375f, .375f);
	SET_TEXT_CENTRE(true);
	END_TEXT_COMMAND_DISPLAY_TEXT(.875f, .95f - .0065f, 0);
	BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
	std::string buffer;
	buffer.resize(sizeof(ball_cur_attempt));
	sprintf(buffer.data(), "%hu", ball_cur_attempt);
	ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(buffer.c_str());
	SET_TEXT_COLOUR(60, 255, 60, 255);
	SET_TEXT_SCALE(.7f, .7f);
	SET_TEXT_RIGHT_JUSTIFY(true);
	SET_TEXT_WRAP(0.f, .95f);
	END_TEXT_COMMAND_DISPLAY_TEXT(.95f, .95f - .026f, 0);
}

static void set_mod_state(enum mod_state mod_state)
{
	::mod_state = mod_state;
	switch (mod_state)
	{
	case mod_state::idle:
		mod_log("Mod state to idle\n");
		break;
	case mod_state::launching_ball:
		mod_log("Mod state to launching ball\n");
		break;
	case mod_state::launching_ball2:
		mod_log("Mod state to launching ball 2\n");
		break;
	case mod_state::spectating_ball:
		mod_log("Mod state to spectating ball\n");
		break;
	default:
		mod_log("Mod state to unknown\n");
		break;
	}
}

static void toggle_mod()
{
	mod_log("Requested mod state change\n");

	auto player_ped        = PLAYER_PED_ID();
	const auto &player_pos = GET_ENTITY_COORDS(player_ped, false);

	if (mod_state == mod_state::idle)
	{
		ballconfig.read_config();

		spawn_ball                  = 0;
		ball_target                 = 0;
		ball_respawn_notify_timer   = 0.f;
		ball_timeout_timer          = ballconfig().ball_timeout_time;
		ball_goal_celebration_timer = 0.f;
		ball_cur_attempt            = 1;
		toggle_ball_respawn         = false;
		toggle_win                  = false;
		float nearest_dist          = ballconfig().ball_spawn_search_dist;
		for (auto prop : get_all_props_array())
		{
			Hash model = GET_ENTITY_MODEL(prop);
			if (!ball_target && model == ballconfig().goal_model_hash)
			{
				ball_target = prop;
				continue;
			}

			if (model != ballconfig().ball_model_hash)
				continue;

			auto pos   = GET_ENTITY_COORDS(prop, false);
			float dist = pos.DistanceTo(player_pos);
			if (dist < nearest_dist)
			{
				spawn_ball   = prop;
				nearest_dist = dist;
			}
		}

		if (spawn_ball)
		{
			mod_log("Found ball %i, using it as spawn point\n", spawn_ball);

			SET_ENTITY_VISIBLE(spawn_ball, false, false);
			SET_ENTITY_COLLISION(spawn_ball, false, false);
			set_ball_spawn_pos(GET_ENTITY_COORDS(spawn_ball, false));
		}
#ifndef NDEBUG
		else
		{
			mod_log("Couldn't find ball, setting spawn point to current pos\n");
			set_ball_spawn_pos(player_pos);
		}
#else
		if (!spawn_ball)
			mod_log("ERROR: No spawn ball found, not changing state\n");
		else
#endif
		{
			respawn_ball();
			set_mod_state(mod_state::launching_ball);
		}
	}
	else
	{
		set_mod_state(mod_state::idle);

		SET_ENTITY_INVINCIBLE(player_ped, false);
		SET_ENTITY_VISIBLE(player_ped, true, false);
		SET_ENTITY_COLLISION(player_ped, true, true);
		FREEZE_ENTITY_POSITION(player_ped, false);

		if (spawn_ball)
		{
			SET_ENTITY_VISIBLE(spawn_ball, true, false);
			SET_ENTITY_COLLISION(spawn_ball, true, true);
		}

		float groundz;
		if (GET_GROUND_Z_FOR_3D_COORD(ball_spawn_pos.x, ball_spawn_pos.y, ball_spawn_pos.z + 10.f, &groundz, false,
		                              false))
			SET_ENTITY_COORDS(player_ped, ball_spawn_pos.x, ball_spawn_pos.y, groundz, false, false, false, false);

		RENDER_SCRIPT_CAMS(false, false, 0, false, false, false);

		delete_ball();
	}
}

static void loop_launch()
{
	if (ballconfig().enable_help_texts)
		DISPLAY_HELP_TEXT_THIS_FRAME(HELP_LAUNCH_HINT, false);

	// Free look
	ENABLE_CONTROL_ACTION(0, 1, true);
	ENABLE_CONTROL_ACTION(0, 2, true);

	SET_ENTITY_COLLISION(ball, false, false);
	const auto &cam_rot = GET_GAMEPLAY_CAM_ROT(0);
	SET_ENTITY_ROTATION(ball, cam_rot.x, cam_rot.y, cam_rot.z, 0, false);

	const auto &launch_vec_norm   = GET_ENTITY_FORWARD_VECTOR(ball);

	// i suck at math, so just use an invisible ball to get trajectory
	static Vector3 cached_forward = { 0.f, 0.f, 0.f };
	static float sim_time         = 0.f;
	static std::vector<Vector3> sim_coords;
	static Object sim_ball = 0;
	if (cached_forward.x != launch_vec_norm.x || cached_forward.y != launch_vec_norm.y
	    || cached_forward.z != launch_vec_norm.z)
	{
		cached_forward = launch_vec_norm;
		sim_time       = ballconfig().ball_launch_trajectory_sim_time / 1000.f;
		sim_coords.clear();
		if (DOES_ENTITY_EXIST(sim_ball))
			DELETE_ENTITY(&sim_ball);
		sim_ball = CREATE_OBJECT(ballconfig().ball_model_hash, ball_spawn_pos.x, ball_spawn_pos.y, ball_spawn_pos.z,
		                         true, false, true);
		SET_ENTITY_VISIBLE(sim_ball, false, false);
		invoke<Void>(0x2AED6301F67007D5, sim_ball); // _DISABLE_CAM_COLLISION_FOR_ENTITY
		APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(sim_ball, 1, launch_vec_norm.x * ballconfig().ball_launch_strength,
		                                     launch_vec_norm.y * ballconfig().ball_launch_strength,
		                                     launch_vec_norm.z * ballconfig().ball_launch_strength, false, false, true,
		                                     false);
	}
	if (sim_time > 0.f)
	{
		sim_coords.push_back(GET_ENTITY_COORDS(sim_ball, false));
		sim_time -= GET_FRAME_TIME();
	}
	else if (sim_ball && DOES_ENTITY_EXIST(sim_ball))
	{
		DELETE_ENTITY(&sim_ball);
		sim_ball = 0;
	}

	for (int i = 1; i < sim_coords.size(); i++)
		DRAW_LINE(sim_coords[i - 1].x, sim_coords[i - 1].y, sim_coords[i - 1].z, sim_coords[i].x, sim_coords[i].y,
		          sim_coords[i].z, 255, 0, 0,
		          sim_time > 0.f
		              ? std::lerp(255, 0, sim_time / (ballconfig().ball_launch_trajectory_sim_time / 1000.f))
		              : std::lerp(
		                  0, 255,
		                  (std::sin(GET_GAME_TIMER() * -.01f + static_cast<float>(i) / sim_coords.size() * 20.f) + 1.f)
		                      * .5f));

	if (ballconfig().enable_attempts_counter)
		display_attempts_counter();

	if (IS_DISABLED_CONTROL_JUST_PRESSED(0, 24))
	{
		::launch_vec_norm = launch_vec_norm;
		last_launch_rot   = cam_rot;
		set_mod_state(mod_state::launching_ball2);
	}
}

static void loop_launch2()
{
	if (!HAS_STREAMED_TEXTURE_DICT_LOADED("CommonMenu"))
		REQUEST_STREAMED_TEXTURE_DICT("CommonMenu", false);
	if (!HAS_STREAMED_TEXTURE_DICT_LOADED("shared"))
		REQUEST_STREAMED_TEXTURE_DICT("shared", false);

	if (ballconfig().enable_help_texts)
		DISPLAY_HELP_TEXT_THIS_FRAME(HELP_LAUNCH2_HINT, false);

	SET_ENTITY_COLLISION(ball, false, false);

	auto &&ball_pos = GET_ENTITY_COORDS(ball, false);

	float lerp      = (std::sin(GET_GAME_TIMER() * .01f) + 1.f) * .5f;
	DRAW_SPRITE("CommonMenu", "Gradient_Bgd", .9f, .6f, .025f, .5f, 0.f, 20, 20, 20, 180, false);
	DRAW_RECT(.9f, std::lerp(.85f, .6f, lerp) - .004f, .02f, std::lerp(0.f, .5f, lerp), std::lerp(255, 0, lerp),
	          std::lerp(0, 255, lerp), 0, 255, false);
	DRAW_RECT(.9f, .6f - std::lerp(-.25f, .25f, lerp), .03f, .01f, std::lerp(255, 0, lerp), std::lerp(0, 255, lerp), 0,
	          255, false);

	if (ballconfig().enable_attempts_counter)
		display_attempts_counter();

	DRAW_LINE(ball_pos.x, ball_pos.y, ball_pos.z, ball_pos.x + launch_vec_norm.x * BALL_LAUNCH_INDICATOR_LEN * lerp,
	          ball_pos.y + launch_vec_norm.y * BALL_LAUNCH_INDICATOR_LEN * lerp,
	          ball_pos.z + launch_vec_norm.z * BALL_LAUNCH_INDICATOR_LEN * lerp, 255, 0, 0, std::lerp(0, 255, lerp));

	if (IS_DISABLED_CONTROL_JUST_RELEASED(0, 25))
		set_mod_state(mod_state::launching_ball);
	else if (IS_DISABLED_CONTROL_JUST_PRESSED(0, 24))
	{
		float launch_strength = lerp * ballconfig().ball_launch_strength;
		SET_ENTITY_COLLISION(ball, true, true);
		APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(ball, 1, launch_vec_norm.x * launch_strength,
		                                     launch_vec_norm.y * launch_strength, launch_vec_norm.z * launch_strength,
		                                     false, false, true, false);

		set_mod_state(mod_state::spectating_ball);
	}
}

static void loop_spectate()
{
	if (ballconfig().enable_help_texts)
		DISPLAY_HELP_TEXT_THIS_FRAME(HELP_CAMERA_HINT, false);

	if (!IS_ENTITY_IN_WATER(ball))
	{
		// Free look
		ENABLE_CONTROL_ACTION(0, 1, true);
		ENABLE_CONTROL_ACTION(0, 2, true);
	}

	const auto &ball_pos   = GET_ENTITY_COORDS(ball, false);
	auto gameplay_cam_mode = GET_FOLLOW_PED_CAM_VIEW_MODE();

	// make sure gameplay cam doesn't get too close
	const auto &cam_pos    = GET_GAMEPLAY_CAM_COORD();
	auto cam_ball_pos_dist = ball_pos.DistanceTo(cam_pos);
	if (gameplay_cam_mode != 4)
	{
		const auto &cam_rot           = GET_GAMEPLAY_CAM_ROT(0);

		static Camera ball_no_col_cam = 0;
		if (!ball_no_col_cam || !DOES_CAM_EXIST(ball_no_col_cam))
			ball_no_col_cam = CREATE_CAM("DEFAULT_SCRIPTED_CAMERA", true);

		auto cam_ball_pos_diff     = cam_pos - ball_pos;
		auto cam_ball_pos_diff_len = cam_ball_pos_diff.Length();
		auto cam_ball_pos_diff_norm =
		    Vector3(cam_ball_pos_diff.x / cam_ball_pos_diff_len, cam_ball_pos_diff.y / cam_ball_pos_diff_len,
		            cam_ball_pos_diff.z / cam_ball_pos_diff_len);
		auto ball_no_col_cam_pos = ball_pos + cam_ball_pos_diff_norm * 1.5f * (gameplay_cam_mode + 1);
		static Vector3 prev_good_ball_no_col_cam_vec = ball_no_col_cam_pos - ball_pos;
		// fall back to previous good cam pos (with updated ball pos) if the new no collision cam pos is obscured
		auto raycast_handle =
		    _START_SHAPE_TEST_RAY(ball_no_col_cam_pos.x, ball_no_col_cam_pos.y, ball_no_col_cam_pos.z, ball_pos.x,
		                          ball_pos.y, ball_pos.z, 19 /* objects, vehicles and world geometry */, ball, 4);
		BOOL has_hit;
		Vector3 end_coords, surface_normal;
		Entity entity_hit;
		GET_SHAPE_TEST_RESULT(raycast_handle, &has_hit, &end_coords, &surface_normal, &entity_hit);
		if (has_hit)
			SET_CAM_COORD(ball_no_col_cam, ball_pos.x + prev_good_ball_no_col_cam_vec.x,
			              ball_pos.y + prev_good_ball_no_col_cam_vec.y, ball_pos.z + prev_good_ball_no_col_cam_vec.z);
		else
		{
			SET_CAM_COORD(ball_no_col_cam, ball_no_col_cam_pos.x, ball_no_col_cam_pos.y, ball_no_col_cam_pos.z);
			prev_good_ball_no_col_cam_vec = ball_no_col_cam_pos - ball_pos;
		}
		SET_CAM_ROT(ball_no_col_cam, cam_rot.x, cam_rot.y, cam_rot.z, 0);
		SET_CAM_FOV(ball_no_col_cam, GET_GAMEPLAY_CAM_FOV());

		if (cam_ball_pos_dist < 1.5f * (gameplay_cam_mode + 1))
		{
			RENDER_SCRIPT_CAMS(true, false, 0, false, false, false);
			// disable camera mode changing
			DISABLE_CONTROL_ACTION(0, 0, true);
		}
		else
			RENDER_SCRIPT_CAMS(false, false, 0, false, false, false);
	}
	else
		RENDER_SCRIPT_CAMS(false, false, 0, false, false, false);

	// ragdoll peds if ball is touching them
	for (auto ped : get_all_peds())
	{
		if (IS_PED_RAGDOLL(ped) || !IS_ENTITY_TOUCHING_ENTITY(ball, ped))
			continue;

		if (ballconfig().peds_are_invincible)
			SET_ENTITY_INVINCIBLE(ped, true);

		const auto &ball_vel   = GET_ENTITY_VELOCITY(ball);
		const auto &force_mult = ballconfig().ball_hit_ped_fling_multiplier;
		SET_PED_TO_RAGDOLL(ped, 3000, 3000, 0, true, true, false);
		int closest_bone        = 0;
		float closest_bone_dist = std::numeric_limits<float>::max();
		for (int i = 0; i < invoke<int>(0xB328DCC3A3AA401B, ped) /* _GET_ENTITY_BONE_COUNT */; i++)
		{
			float bone_dist = ball_pos.DistanceTo(_GET_ENTITY_BONE_COORDS(ped, i));
			if (bone_dist < closest_bone_dist)
			{
				closest_bone      = i;
				closest_bone_dist = bone_dist;
			}
		}
		APPLY_FORCE_TO_ENTITY(ped, 3, ball_vel.x * force_mult, ball_vel.y * force_mult, ball_vel.z * force_mult, 0.f,
		                      0.f, 0.f, closest_bone, false, false, true, false, false);
	}

	float frame_time   = GET_FRAME_TIME();

	bool is_timing_out = false;
	if (!toggle_ball_respawn && ball_respawn_notify_timer <= 0.f)
	{
		/* raycast first, since even though it's more expensive we want to prioritize
		the out of bounds message over the stuck message */
		if (ballconfig().enable_out_of_bounds_check)
		{
			int ray_handle = _START_SHAPE_TEST_RAY(ball_pos.x, ball_pos.y, ball_pos.z, ball_pos.x, ball_pos.y,
			                                       ball_pos.z - 10.f, 16 /* props */, ball, 4);
			BOOL has_hit;
			Vector3 end_coords, surface_normal;
			Entity entity_hit;
			if (GET_SHAPE_TEST_RESULT(ray_handle, &has_hit, &end_coords, &surface_normal, &entity_hit) == 2 && !has_hit)
			{
				ball_respawn_reason = ball_respawn_reason::out_of_bounds;
				is_timing_out       = true;
			}
		}

		static Vector3 ball_last_pos;
		if (!is_timing_out)
		{
			if (ball_last_pos.DistanceTo(ball_pos) < .05f)
			{
				ball_respawn_reason = ball_respawn_reason::stuck;
				is_timing_out       = true;
			}
		}
		ball_last_pos = ball_pos;
	}

	static int cached_ball_attempts = 0;
	if (toggle_win
	    || (ball_target && IS_ENTITY_TOUCHING_ENTITY(ball, ball_target) && ball_goal_celebration_timer <= 0.f))
	{
		if (toggle_win)
			mod_log("Target hit (toggle_win)!\n");
		else
			mod_log("Target hit!\n");
		mod_log("Entering celebration mode\n");

		cached_ball_attempts        = ball_cur_attempt;
		ball_cur_attempt            = 0;

		toggle_win                  = false;
		toggle_ball_respawn         = true;
		ball_goal_celebration_timer = ballconfig().ball_goal_celebration_time;
		ball_respawn_reason         = ball_respawn_reason::hit_target;

		SET_ENTITY_VISIBLE(ball, false, false);
		FREEZE_ENTITY_POSITION(ball, true);
	}

	static bool played_sound     = false;
	static int spawned_fireworks = 0;
	if (ball_goal_celebration_timer > 0.f)
	{
		ball_goal_celebration_timer -= GET_FRAME_TIME();

		if (!played_sound && REQUEST_MISSION_AUDIO_BANK("DLC_HALLOWEEN/TG_01", false, 0))
		{
			PLAY_SOUND_FRONTEND(-1, "Cheers", "DLC_TG_Running_Back_Sounds", false);
			played_sound = true;
		}

		static float spawn_firework_time = 0.f;
		auto game_time                   = GET_GAME_TIMER();
		if (game_time > spawn_firework_time && spawned_fireworks < 5)
		{
			if (!HAS_NAMED_PTFX_ASSET_LOADED("proj_indep_firework_v2"))
				REQUEST_NAMED_PTFX_ASSET("proj_indep_firework_v2");
			else
			{
				USE_PARTICLE_FX_ASSET("proj_indep_firework_v2");
				auto firework_pos = Vector3(ball_pos.x + GET_RANDOM_FLOAT_IN_RANGE(-10.f, 10.f),
				                            ball_pos.y + GET_RANDOM_FLOAT_IN_RANGE(-10.f, 10.f), ball_pos.z);
				START_PARTICLE_FX_NON_LOOPED_AT_COORD("scr_firework_indep_ring_burst_rwb", firework_pos.x,
				                                      firework_pos.y, firework_pos.z, 0.0f, 0.0f, 0.0f, 2.0f, true,
				                                      true, true);
				spawned_fireworks++;
				spawn_firework_time = game_time + 400;
			}
		}

		BEGIN_SCALEFORM_MOVIE_METHOD(notify_scaleform, "SHOW_SHARD_RANKUP_MP_MESSAGE");
		SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_SUCCESS));
		std::string buffer;
		buffer.resize(32);
		sprintf(buffer.data(), _GET_LABEL_TEXT(HELP_SUCCESS_ATTEMPTS_PREFIX), cached_ball_attempts);
		SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(buffer.c_str());
		END_SCALEFORM_MOVIE_METHOD();
		DRAW_SCALEFORM_MOVIE_FULLSCREEN(notify_scaleform, 255, 255, 255, 255, 0);

		ANIMPOSTFX_PLAY("RaceTurbo", 1000, false);

		// disable camera mode changing
		DISABLE_CONTROL_ACTION(0, 0, true);

		// set to third person cam if in first person during celebration
		if (gameplay_cam_mode == 4)
			SET_FOLLOW_PED_CAM_VIEW_MODE(1);
	}
	else
	{
		played_sound      = false;
		spawned_fireworks = 0;
		if (is_timing_out)
			ball_timeout_timer -= frame_time;
		else
			ball_timeout_timer = ballconfig().ball_respawn_timeout;
	}

	if (ball_goal_celebration_timer <= 0.f && (toggle_ball_respawn || ball_timeout_timer <= 0.f))
	{
		toggle_ball_respawn = false;
		ball_timeout_timer  = ballconfig().ball_timeout_time;
		if (ballconfig().notify_on_manual_respawn || ball_respawn_reason != ball_respawn_reason::manually)
			ball_respawn_notify_timer = ballconfig().ball_respawn_notify_time;
		ball_cur_attempt++;
		mod_log("Requesting ball respawn\n");
		respawn_ball();
		set_mod_state(mod_state::launching_ball);
	}
}

static void loop_main()
{
	if (toggle_mod_state)
	{
		toggle_mod_state = false;
		toggle_mod();
	}

	if (mod_state == mod_state::idle)
		return;

	auto player_ped = PLAYER_PED_ID();

	SET_ENTITY_INVINCIBLE(player_ped, true);
	SET_ENTITY_VISIBLE(player_ped, false, false);
	SET_ENTITY_COLLISION(player_ped, false, false);
	FREEZE_ENTITY_POSITION(player_ped, true);

	const auto &ball_pos = GET_ENTITY_COORDS(ball, false);
	if (!IS_ENTITY_IN_WATER(ball))
		SET_ENTITY_COORDS(player_ped, ball_pos.x, ball_pos.y, ball_pos.z - 1.2f, false, false, false, false);

	// rotate player to gameplay cam rotation so the camera angles aren't clamped in first person
	const auto &cam_rot = GET_GAMEPLAY_CAM_ROT(0);
	SET_ENTITY_ROTATION(player_ped, cam_rot.x, cam_rot.y, cam_rot.z, 0, true);

	DISABLE_ALL_CONTROL_ACTIONS(0);
	ENABLE_CONTROL_ACTION(0, 0, true);   // Next camera
	ENABLE_CONTROL_ACTION(0, 199, true); // Pause
	ENABLE_CONTROL_ACTION(0, 200, true); // Pause2
	HIDE_HUD_AND_RADAR_THIS_FRAME();

	if (ball_respawn_notify_timer > 0.f)
	{
		ball_respawn_notify_timer -= GET_FRAME_TIME();

		BEGIN_SCALEFORM_MOVIE_METHOD(notify_scaleform, "SHOW_SHARD_RANKUP_MP_MESSAGE");
		switch (ball_respawn_reason)
		{
		case ball_respawn_reason::out_of_bounds:
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_BALL_OUT_OF_COURSE));
			break;
		case ball_respawn_reason::stuck:
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_BALL_STUCK));
			break;
		case ball_respawn_reason::manually:
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_BALL_MANUAL));
			break;
		case ball_respawn_reason::hit_target:
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_BALL_HIT_TARGET));
			break;
		default:
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING("UNKNOWN");
			break;
		}
		if (ball_respawn_reason != ball_respawn_reason::hit_target)
			SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(_GET_LABEL_TEXT(HELP_BALL_RESPAWN_SUBTEXT));
		END_SCALEFORM_MOVIE_METHOD();
		DRAW_SCALEFORM_MOVIE_FULLSCREEN(notify_scaleform, 255, 255, 255, 255, 0);
	}

	switch (mod_state)
	{
	case mod_state::launching_ball:
		loop_launch();
		break;
	case mod_state::launching_ball2:
		loop_launch2();
		break;
	case mod_state::spectating_ball:
		loop_spectate();
		break;
	default:
		break;
	}
}

static void keyboard_handle(DWORD key, WORD repeats, BYTE scancode, BOOL is_extended, BOOL with_alt, BOOL was_down,
                            BOOL is_up)
{
	static bool is_ctrl_down = false;
	if (key == VK_CONTROL)
		is_ctrl_down = !is_up;

	if (was_down)
		return;

	if (is_ctrl_down)
	{
		if (key == 0x4c) // l
			toggle_mod_state = true;

		if (key == 0x4f && mod_state == mod_state::spectating_ball) // o
		{
			toggle_ball_respawn = true;
			ball_respawn_reason = ball_respawn_reason::manually;
		}

#ifndef NDEBUG
		if (key == 0x4d && mod_state == mod_state::spectating_ball && ball_goal_celebration_timer <= 0.f) // m
			toggle_win = true;
#endif
	}
}

namespace ballgame
{
	void script_main()
	{
		mod_log("Mod started up!\n");

		notify_scaleform = REQUEST_SCALEFORM_MOVIE("MP_BIG_MESSAGE_FREEMODE");
		while (true)
		{
			loop_main();
			scriptWait(0);
		}
	}

	void keyboard_handle(DWORD key, WORD repeats, BYTE scancode, BOOL is_extended, BOOL with_alt, BOOL was_down,
	                     BOOL is_up)
	{
		::keyboard_handle(key, repeats, scancode, is_extended, with_alt, was_down, is_up);
	}
}