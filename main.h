#pragma once

using BOOL  = int;
using BYTE  = unsigned char;
using WORD  = unsigned short;
using DWORD = long unsigned int;

namespace ballgame
{
	void script_main();
	void keyboard_handle(DWORD key, WORD repeats, BYTE scancode, BOOL is_extended, BOOL with_alt, BOOL was_down,
	                     BOOL is_up);
}