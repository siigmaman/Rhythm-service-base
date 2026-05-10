#pragma once

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/formats/json.hpp>

namespace rhythm::handlers::http_json {

[[nodiscard]] inline std::string JsonError(std::string_view code) {
    userver::formats::json::ValueBuilder err;
    err["error"] = std::string{code};
    return userver::formats::json::ToString(err.ExtractValue());
}

[[nodiscard]] inline std::string JsonActionAccepted(bool duplicate) {
    userver::formats::json::ValueBuilder ok;
    ok["status"] = "accepted";
    ok["duplicate"] = duplicate;
    return userver::formats::json::ToString(ok.ExtractValue());
}

[[nodiscard]] inline std::string TrimCopy(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return std::string{s};
}

[[nodiscard]] inline std::string JsonFieldAsString(const userver::formats::json::Value& json, const char* key) {
    if (!json.HasMember(key)) {
        throw std::runtime_error(std::string("missing field '") + key + "'");
    }
    const auto& v = json[key];
    if (v.IsString()) return v.As<std::string>();
    if (v.IsInt64()) return std::to_string(v.As<std::int64_t>());
    if (v.IsUInt64()) return std::to_string(static_cast<std::int64_t>(v.As<std::uint64_t>()));
    if (v.IsDouble()) return std::to_string(v.As<double>());
    throw std::runtime_error(std::string("field '") + key + "' has unsupported type");
}

}  // namespace rhythm::handlers::http_json
