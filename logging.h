#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <iomanip>
#include <string>
#include <string_view>

inline const auto mod_start_time = std::time(nullptr);

inline void golf_log_raw(std::string_view text)
{
	static auto log_file = []
	{
		auto file = fopen("golfgame.log", "w");
		fclose(file);
		file = fopen("golfgame.log", "a");
		return file;
	}();

	fprintf(log_file, text.data());
	fflush(log_file);
}

inline void mod_log(std::string_view fmt, auto &&...args)
{
	auto cur_time       = std::time(nullptr);
	auto diff_time      = static_cast<time_t>(std::difftime(cur_time, mod_start_time));
	auto diff_time_time = *std::gmtime(&diff_time);

	std::string prep_buffer;
	prep_buffer.resize(128);
	std::stringstream time;
	time << std::put_time(&diff_time_time, "%H:%M:%S");
	const auto &time_str = time.str();
	sprintf(prep_buffer.data(), "[%s] %s", time_str.c_str(), fmt.data());

	std::string buffer;
	buffer.resize(128);
	sprintf(buffer.data(), prep_buffer.data(), args...);

	golf_log_raw(buffer);
}