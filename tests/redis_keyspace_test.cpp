#include <gtest/gtest.h>

#include "infra/redis_keyspace.hpp"

TEST(RedisKeyspace, FeedKeyUsesHashTag) { EXPECT_EQ(rhythm::infra::redis_keys::FeedKey("u1"), "{rhythm:u1}:feed"); }

TEST(RedisKeyspace, VisitLastSharesUserTag) {
    EXPECT_EQ(rhythm::infra::redis_keys::VisitLastKey("u1"), "{rhythm:u1}:visit_last");
}

TEST(RedisKeyspace, SimilarKey) { EXPECT_EQ(rhythm::infra::redis_keys::SimilarKey(7), "{rhythm:similar}:7"); }

TEST(RedisKeyspace, PopularGlobalKey) {
    EXPECT_EQ(rhythm::infra::redis_keys::PopularGlobalKey(), "{rhythm}:popular_global");
}

TEST(RedisKeyspace, IdempotencyKey) {
    EXPECT_EQ(rhythm::infra::redis_keys::IdempotencyKey("tok"), "{rhythm:idempotency}:tok");
}
