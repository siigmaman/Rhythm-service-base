#include <gtest/gtest.h>

#include "infra/in_memory_stubs.hpp"

TEST(InMemoryRhythmData, IdempotencySecondCallDuplicate) {
    rhythm::infra::InMemoryRhythmData data;
    EXPECT_TRUE(data.TryReserve("tok", std::chrono::hours{1}));
    EXPECT_FALSE(data.TryReserve("tok", std::chrono::hours{1}));
}

TEST(InMemoryRhythmData, IdempotencyEmptyTokenRejected) {
    rhythm::infra::InMemoryRhythmData data;
    EXPECT_FALSE(data.TryReserve("", std::chrono::hours{1}));
}

TEST(InMemoryRhythmData, FeedRoundTrip) {
    rhythm::infra::InMemoryRhythmData data;
    const std::vector<std::int64_t> ids{1, 2, 3};
    data.PutFeed("u", ids, std::chrono::hours{24});
    auto got = data.TryGetFeed("u");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->product_ids, ids);
}

TEST(InMemoryRhythmData, DropFeed) {
    rhythm::infra::InMemoryRhythmData data;
    data.PutFeed("u", {1}, std::chrono::hours{1});
    data.DropFeed("u");
    EXPECT_FALSE(data.TryGetFeed("u").has_value());
}
