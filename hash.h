#pragma once

#include "natives.h"

constexpr Hash operator""_hash(const char *str, size_t n)
{
	return GET_HASH_KEY(str);
}

static_assert("prop_golf_ball"_hash == 0xaf0e3f9f);
static_assert("PROP_GOLF_BALL"_hash == 0xaf0e3f9f);