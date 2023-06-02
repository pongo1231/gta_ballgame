#pragma once

#include "logging.h"

#include <exception>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <type_traits>
#include <unordered_map>

class config
{
	std::unordered_map<std::string, std::string> options;

  public:
	config() = default;

	config(std::string_view file_name)
	{
		mod_log("Parsing config file %s...\n", file_name.data());

		struct stat temp;
		if (stat(file_name.data(), &temp))
		{
			mod_log("ERROR: Config file %s not found!", file_name.data());
			return;
		}

		auto file = fopen(file_name.data(), "r");
		fseek(file, 0, SEEK_END);
		auto file_size = ftell(file);
		rewind(file);

		std::string buffer;
		buffer.resize(file_size);
		fread(buffer.data(), sizeof(char), file_size, file);

		while (!buffer.empty())
		{
			auto endl   = buffer.find('\n');
			auto line   = buffer.substr(0, endl);

			auto equate = line.find('=');
			// skip if line is a comment or empty
			if ((line.find('#') == line.npos || line.find_first_not_of(' ') != line.npos
			         ? line[line.find_first_not_of(' ')] != '#'
			         : false)
			    && equate != line.npos)
			{
				auto trim = [](std::string str) -> std::string
				{
					if (str.find_first_not_of(' ') == str.npos)
						return nullptr;
					str = str.substr(str.find_first_not_of(' '));
					str = str.substr(0, str.find_last_not_of(' ') + 1);
					return str;
				};
				auto key   = trim(line.substr(0, equate));
				auto value = trim(line.substr(equate + 1));

				if (!key.empty() && !value.empty())
				{
					mod_log("%s set to %s\n", key.c_str(), value.c_str());
					options[key] = value;
				}
			}

			if (endl == buffer.npos)
				break;
			buffer = buffer.substr(endl + 1);
		}

		mod_log("Parsed config file %s!\n", file_name.data());
	}

  public:
	template <typename t> t get_value(const std::string &key) const
	{
		if (options.contains(key))
		{
			const auto &value = options.at(key);
			try
			{
				return std::stof(value);
			}
			catch (const std::exception &ex)
			{
				mod_log("ERROR: Couldn't parse key %s (value %s)\n", key.c_str(), value.c_str());
			}
		}

		return 0;
	}
};

template <> inline std::string config::get_value<std::string>(const std::string &key) const
{
	return options.contains(key) ? options.at(key) : "";
}