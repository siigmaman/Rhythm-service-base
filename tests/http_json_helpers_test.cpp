#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <userver/formats/json.hpp>

#include "handlers/http_json_helpers.hpp"

TEST(HttpJsonHelpers, TrimCopy) {
    EXPECT_EQ(rhythm::handlers::http_json::TrimCopy("  abc  "), "abc");
    EXPECT_EQ(rhythm::handlers::http_json::TrimCopy(""), "");
}

TEST(HttpJsonHelpers, JsonFieldAsStringFromString) {
    const auto j = userver::formats::json::FromString(R"({"x":"hi"})");
    EXPECT_EQ(rhythm::handlers::http_json::JsonFieldAsString(j, "x"), "hi");
}

TEST(HttpJsonHelpers, JsonFieldAsStringFromInt) {
    const auto j = userver::formats::json::FromString(R"({"x":42})");
    EXPECT_EQ(rhythm::handlers::http_json::JsonFieldAsString(j, "x"), "42");
}

TEST(HttpJsonHelpers, JsonFieldMissingThrows) {
    const auto j = userver::formats::json::FromString(R"({})");
    EXPECT_THROW(rhythm::handlers::http_json::JsonFieldAsString(j, "nope"), std::runtime_error);
}

TEST(HttpJsonHelpers, JsonErrorShape) {
    const auto s = rhythm::handlers::http_json::JsonError(std::string_view{"bad"});
    const auto j = userver::formats::json::FromString(s);
    EXPECT_EQ((j["error"].As<std::string>()), "bad");
}
