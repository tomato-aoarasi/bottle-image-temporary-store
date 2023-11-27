/*
 * @File	  : config.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:14
 * @Introduce : 配置类(解析yaml)
*/

#pragma once

#include <fstream>
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"
#include "fmt/format.h"
#include <limits>

#ifndef CONFIG_HPP
#define CONFIG_HPP  

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace std {
	using fmt::format;
	using fmt::format_error;
	using fmt::formatter;
}

using nlohmann::json;


namespace self {
	namespace define {

	};
};

namespace Config {
	static YAML::Node config_yaml{ YAML::LoadFile("./config.yaml") };
};

namespace Global {
	inline std::string resource_path{ Config::config_yaml["server"]["resource-path"].as<std::string>() };
	inline std::string out_path{ Config::config_yaml["server"]["out-path"].as<std::string>() };
	inline std::string auth{ Config::config_yaml["server"]["auth"].as<std::string>() };
};

namespace Config {
	void initialized(void) {
		
	}
}

#endif