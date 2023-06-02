#pragma once

#include "natives.h"

namespace cam
{
	static Vector3 deg_to_rad(const Vector3 &angles)
	{
		Vector3 vec;
		vec.x = angles.x * .0174532925199433f;
		vec.y = angles.y * .0174532925199433f;
		vec.z = angles.z * .0174532925199433f;

		return vec;
	}

	static Vector3 rot_to_direction(Vector3 *rot)
	{
		auto radians_z = rot->z * 0.0174532924f;
		auto radians_x = rot->x * 0.0174532924f;
		auto num       = std::abs((float)std::cos((double)radians_x));
		Vector3 dir;
		dir.x = (float)((double)((float)(-(float)std::sin((double)radians_z))) * (double)num);
		dir.y = (float)((double)((float)std::cos((double)radians_z)) * (double)num);
		dir.z = (float)std::sin((double)radians_x);
		return dir;
	}

	static Vector3 get_coords_from_gameplay_cam(float distance)
	{
		auto rot    = deg_to_rad(GET_GAMEPLAY_CAM_ROT(2));
		auto coords = GET_GAMEPLAY_CAM_COORD();

		rot.y       = distance * cos(rot.x);
		coords.x    = coords.x + rot.y * std::sin(rot.z * -1.f);
		coords.y    = coords.y + rot.y * std::cos(rot.z * -1.f);
		coords.z    = coords.z + distance * sin(rot.x);

		return coords;
	}
}