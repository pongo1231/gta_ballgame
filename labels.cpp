#include <stdafx.h>

#include "labels.h"

using Hash = unsigned long;

static std::unordered_map<Hash, const char *> labels_map;

const char *(*og_get_label_text)(void *, Hash);
const char *hk_get_label_text(void *text, Hash hash)
{
	const auto &result = labels_map.find(hash);
	if (result != labels_map.end())
	{
		return result->second;
	}

	return og_get_label_text(text, hash);
}

namespace labels
{
	void init_labels()
	{
		auto handle = memory::find_pattern("48 8B CB 8B D0 E8 ? ? ? ? 48 85 C0 0F 95 C0");
		if (!handle.is_valid())
			mod_log("labels func 1 hook failed\n");
		else
		{
			handle = handle.at(5).into();
			MH_CreateHook(handle.get<void>(), hk_get_label_text, (void **)&og_get_label_text);

			handle = memory::find_pattern("48 85 C0 75 34 8B 0D");
			if (!handle.is_valid())
				mod_log("labels func 2 hook failed\n");
			else
			{
				handle = handle.at(-5).into();
				MH_CreateHook(handle.get<void>(), hk_get_label_text, NULL);
			}
		}

		add_label(HELP_LAUNCH_HINT, "Adjust the angle using the camera.\nPress ~INPUT_NEXT_CAMERA~ to change the "
		                            "camera angle.\nPress ~INPUT_ATTACK~ to continue.");
		add_label(HELP_LAUNCH2_HINT,
		          "The bar on the right indicates the strength the ball gets launched with.\nPress ~INPUT_NEXT_CAMERA~ "
		          "to change the camera angle.\nPress ~INPUT_AIM~ to go back.\nPress ~INPUT_ATTACK~ to launch ball.");
		add_label(HELP_CAMERA_HINT, "Press ~INPUT_NEXT_CAMERA~ to change the camera angle.");
		add_label(HELP_BALL_RESPAWN_SUBTEXT, "Try Again!");
		add_label(HELP_BALL_OUT_OF_COURSE, "BALL OUT OF BOUNDS");
		add_label(HELP_BALL_STUCK, "BALL STUCK");
		add_label(HELP_BALL_MANUAL, "BALL RESPAWNED");
		add_label(HELP_BALL_HIT_TARGET, "NEW ROUND");
		add_label(HELP_ATTEMPT, "ATTEMPT");
		add_label(HELP_SUCCESS, "SCORE!");
		add_label(HELP_SUCCESS_ATTEMPTS_PREFIX, "Attempts: ~g~%i");
	}

	void add_label(const char *key, const char *label)
	{
		labels_map[GET_HASH_KEY(key)] = label;
	}
}