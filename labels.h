#pragma once

#define HELP_LAUNCH_HINT "_help_launch_ball"
#define HELP_LAUNCH2_HINT "_help_launch2_ball"
#define HELP_CAMERA_HINT "_help_camera_hint"
#define HELP_BALL_RESPAWN_SUBTEXT "_help_ball_tryagain"
#define HELP_BALL_OUT_OF_COURSE "_help_ball_respawned"
#define HELP_BALL_STUCK "_help_ball_respawned2"
#define HELP_BALL_MANUAL "_help_ball_respawned3"
#define HELP_BALL_HIT_TARGET "_help_ball_respawned4"
#define HELP_ATTEMPT "_help_attempt"
#define HELP_SUCCESS "_help_success"
#define HELP_SUCCESS_ATTEMPTS_PREFIX "_help_success2"

namespace labels
{
	void init_labels();
	void add_label(const char *key, const char *label);
}