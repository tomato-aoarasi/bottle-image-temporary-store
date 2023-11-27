#pragma once

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#include <chrono> 
#include <filesystem>
#include <functional>
#include "crow.h"

#include "spdlog/spdlog.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}
#include "configuration/config.hpp"
#include "common/utils.hpp"
#include "common/self_exception.hpp"
#include "common/http_util.hpp"
#include "common/log_system.hpp"
#include "self/reusable.hpp"

#ifndef MAIN_H
#define MAIN_H

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

using namespace std::literals::string_view_literals;
using namespace std::chrono_literals;

//初始化
void init(void) {
    Config::initialized();
    LogSystem::initialized();
}

inline void checkAuthorization(const crow::request& req){
    auto authentication{ req.get_header_value("Authorization")};
    if ("Bearer " + Global::auth != authentication){
        throw self::HTTPException(401);
    }
}

// 启动项
inline void start(void) {
    init();

    uint16_t port{ Config::config_yaml["server"]["port"].as<uint16_t>() };

    crow::SimpleApp app;
    //Authorization: Bearer 
    CROW_ROUTE(app, "/upload")
        .methods(crow::HTTPMethod::Post)
        ([](const crow::request& req) {
        constexpr const char* filename{ "file" };
        return self::HandleResponseBody([&] {
            checkAuthorization(req);
            if (req.body.empty()) { throw self::HTTPException(400, "form-data is empty"s); }

            crow::multipart::message file_message(req);

            for (const auto& part : file_message.part_map)
            {
                const auto& part_name = part.first;
                const auto& part_value = part.second;
                CROW_LOG_DEBUG << "Part: " << part_name;
                if (filename == part_name)
                {
                    // Extract the file name
                    auto headers_it = part_value.headers.find("Content-Disposition");
                    if (headers_it == part_value.headers.end())
                    {
                        throw self::HTTPException(400, "No Content-Disposition found"s);
                    }
                    auto params_it = headers_it->second.params.find("filename");
                    if (params_it == headers_it->second.params.end())
                    {
                        throw self::HTTPException(400, std::format("Part with name \"{}\" should have a file", filename));
                    }
                    const std::string outfile_name = params_it->second;

                    for (const auto& part_header : part_value.headers)
                    {
                        const auto& part_header_name = part_header.first;
                        const auto& part_header_val = part_header.second;
                        CROW_LOG_DEBUG << "Header: " << part_header_name << '=' << part_header_val.value;
                        for (const auto& param : part_header_val.params)
                        {
                            const auto& param_key = param.first;
                            const auto& param_val = param.second;
                            CROW_LOG_DEBUG << " Param: " << param_key << ',' << param_val;
                        }
                    }

                    std::ofstream outfile(Global::out_path + "/" + outfile_name, std::ios::binary);

                    if (outfile.is_open()) {
                        outfile.write(part_value.body.c_str(), part_value.body.size()); // 将二进制数据写入文件  
                        outfile.close();  
                    }
                    else {
                        outfile.close();
                        throw self::HTTPException(500, "There is an issue with the output path"s);
                    }

                    CROW_LOG_INFO << " Contents written to " << outfile_name;
                }
                else throw self::HTTPException(400, std::format("argu passed by {} is empty", filename));
            }
            return "ok";
            });
            });


    // 图标
    CROW_ROUTE(app, "/favicon.ico").methods(crow::HTTPMethod::Get)([&]() {
        crow::response response;

        std::string path{ Global::resource_path + "favicon.ico"s};

        if (!std::filesystem::exists(path)) {
            response.set_header("Content-Type", "application/json");
            response.code = 404;
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(404).dump(2));
            return response;
        }

        // 获取当前时间
        auto now{ std::chrono::system_clock::now() };

        // 计算七天后的时间
        auto seven_days_later{ now + std::chrono::hours(24 * 7) };

        // 将时间转换为时间戳（秒数）
        auto timestamp{ std::chrono::system_clock::to_time_t(seven_days_later) };

        // 将时间戳转换为 Crow 框架中的 Expires 数值
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&timestamp), "%a, %d %b %Y %H:%M:%S GMT");
        std::string expires{ ss.str() };
        response.set_static_file_info(path);
        response.set_header("Cache-Control", "public");
        response.set_header("Expires", expires);
        return response;
    });

    app.port(port).multithreaded().run_async();

    return;
}

#endif