#pragma once

#include "handle.h"
#include "natives.h"

#define WIN32_LEAN_AND_MEAN
#include <psapi.h>
#include <windows.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using Hash = DWORD;

inline DWORD64 base_addr;
inline DWORD64 end_addr;

inline std::vector<Hash> cached_weapons;
inline std::vector<Hash> cached_ped_models;

namespace memory
{
	inline handle find_pattern(const std::string_view pattern)
	{
		std::vector<int> bytes;

		auto sub   = pattern;
		int offset = 0;

		while ((offset = sub.find(' ')) != sub.npos)
		{
			auto byte_str = sub.substr(0, offset);

			if (byte_str == "?" || byte_str == "??")
			{
				bytes.push_back(-1);
			}
			else
			{
				bytes.push_back(std::stoi(std::string(byte_str), nullptr, 16));
			}

			sub = sub.substr(offset + 1);
		}
		if ((offset = pattern.rfind(' ')) != sub.npos)
		{
			auto byte_str = pattern.substr(offset + 1);
			bytes.push_back(std::stoi(std::string(byte_str), nullptr, 16));
		}

		if (bytes.empty())
		{
			return handle();
		}

		if (!base_addr || !end_addr)
		{
			MODULEINFO module_info {};
			GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &module_info, sizeof(module_info));

			base_addr = reinterpret_cast<DWORD64>(module_info.lpBaseOfDll);
			end_addr  = base_addr + module_info.SizeOfImage;
		}

		int count = 0;
		for (auto addr = base_addr; addr < end_addr; addr++)
		{
			if (bytes[count] == -1 || *reinterpret_cast<BYTE *>(addr) == bytes[count])
			{
				if (++count == bytes.size())
				{
					return handle(addr - count + 1);
				}
			}
			else
			{
				count = 0;
			}
		}

		return handle();
	}

	inline std::vector<Hash> get_all_veh_models()
	{
		static std::vector<Hash> c_rgVehModels;

		if (c_rgVehModels.empty())
		{
			handle handle;

			handle = find_pattern("48 8B 05 ?? ?? ?? ?? 48 8B 14 D0 EB 0D 44 3B 12");
			if (!handle.is_valid())
			{
				return c_rgVehModels;
			}

			handle               = handle.at(2).into();
			DWORD64 ullModelList = handle.value<DWORD64>();

			handle               = find_pattern("0F B7 05 ?? ?? ?? ?? 44 8B 49 18 45 33 D2 48 8B F1");
			if (!handle.is_valid())
			{
				return c_rgVehModels;
			}

			handle           = handle.at(2).into();
			WORD usMaxModels = handle.value<WORD>();

			for (WORD usIdx = 0; usIdx < usMaxModels; usIdx++)
			{
				DWORD64 ullEntry = *reinterpret_cast<DWORD64 *>(ullModelList + 8 * usIdx);
				if (!ullEntry)
				{
					continue;
				}

				Hash ulModel = *reinterpret_cast<Hash *>(ullEntry);

				if (IS_MODEL_VALID(ulModel) && IS_MODEL_A_VEHICLE(ulModel))
				{
					c_rgVehModels.push_back(ulModel);
				}
			}
		}

		return c_rgVehModels;
	}

	inline std::vector<Hash> get_all_ped_models()
	{
		if (cached_ped_models.empty())
		{
			handle handle;

			// TODO: Fix these patterns

			handle = find_pattern("48 8B 05 ? ? ? ? 4C 8B 14 D0 EB 09 41 3B 0A 74 54 4D 8B 52 08 4D 85 D2 75 F2 4D 8B "
			                      "D1 4D 85 D2 74 58 41 0F B7 02 4D 85 DB 74 03 41 89 03");
			if (!handle.is_valid())
			{
				return cached_ped_models;
			}

			auto qword_7FF69DB37F30 = handle.at(2).into().value<DWORD64>();

			handle = find_pattern("0F B7 05 ? ? ? ? 45 33 C9 4C 8B DA 66 85 C0 0F 84 ? ? ? ? 44 0F B7 C0 33 D2 8B C1 "
			                      "41 F7 F0 48 8B 05 ? ? ? ? 4C 8B 14 D0 EB 09 41 3B 0A 74 54 4D 8B 52 08");
			if (!handle.is_valid())
			{
				return cached_ped_models;
			}

			auto word_7FF69DB37F38 = handle.at(2).into().value<WORD>();

			handle = find_pattern("4C 0F AF 05 ? ? ? ? 4C 03 05 ? ? ? ? EB 09 49 83 C2 04 EB B2 4D 8B C1 4D 85 C0 74 "
			                      "03 4D 8B 08 49 8B C1 C3");
			if (!handle.is_valid())
			{
				return cached_ped_models;
			}

			auto qword_7FF69DB37EE8 = handle.at(3).into().value<DWORD64>();
			auto qword_7FF69DB37ED0 = handle.at(10).into().value<DWORD64>();

			handle = find_pattern("3B 05 ? ? ? ? 7D 35 4C 8B C0 83 E0 1F 8B D0 48 8B 05 ? ? ? ? 49 8B C8 48 C1 E9 05 "
			                      "8B 0C 88 0F A3 D1 73 17 4C 0F AF 05");
			if (!handle.is_valid())
			{
				return cached_ped_models;
			}

			auto dword_7FF69DB37ED8 = handle.at(1).into().value<DWORD>();
			auto qword_7FF69DB37F00 = handle.at(18).into().value<DWORD64>();

			for (WORD i = 0; i < word_7FF69DB37F38; i++)
			{
				auto *model = *reinterpret_cast<Hash **>(qword_7FF69DB37F30 + 8 * i);
				if (!model)
				{
					continue;
				}

				// These will crash the game, avoid at all costs !!!!
				const Hash bad_models[] = { 0x2D7030F3, 0x3F039CBA, 0x856CFB02 };

				if (std::find(std::begin(bad_models), std::end(bad_models), *model) == std::end(bad_models))
				{
					__int64 v2 = 0;

					auto v5    = *reinterpret_cast<WORD *>(model + 4);
					LONG v6;
					// IDA copy paste ftw
					if (static_cast<int>(v5) < dword_7FF69DB37ED8
					    && (v6 = *reinterpret_cast<DWORD *>(qword_7FF69DB37F00 + 4 * (v5 >> 5)),
					        _bittest(&v6, v5 & 0x1F)))
					{
						v2 = *reinterpret_cast<__int64 *>(
							qword_7FF69DB37ED0
							+ qword_7FF69DB37EE8 * *reinterpret_cast<WORD *>(reinterpret_cast<__int64>(model) + 4));
					}

					if (v2 && (*reinterpret_cast<BYTE *>(v2 + 157) & 31) == 6) // is a ped model
					{
						cached_ped_models.push_back(*model);
					}
				}
			}
		}
		return cached_ped_models;
	}
}