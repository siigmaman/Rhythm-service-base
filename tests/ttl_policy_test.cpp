#include <gtest/gtest.h>

#include "domain/ttl_policy.hpp"

TEST(TtlPolicy, ShouldInvalidateFeedNullopt) { EXPECT_TRUE(rhythm::ttl::ShouldInvalidateFeed(std::nullopt)); }

TEST(TtlPolicy, ExtendFeedTtlOnHitToday) {
    const auto s = rhythm::ttl::ExtendFeedTtlOnHit(rhythm::ttl::VisitRecency::Today);
    EXPECT_EQ(s, std::chrono::duration_cast<std::chrono::seconds>(rhythm::ttl::kExtendIfVisitedToday));
}

TEST(TtlPolicy, ExtendFeedTtlOnHitYesterday) {
    const auto s = rhythm::ttl::ExtendFeedTtlOnHit(rhythm::ttl::VisitRecency::Yesterday);
    EXPECT_EQ(s, std::chrono::duration_cast<std::chrono::seconds>(rhythm::ttl::kExtendIfVisitedYesterday));
}

TEST(TtlPolicy, ExtendFeedTtlOnHitStale) {
    EXPECT_EQ(rhythm::ttl::ExtendFeedTtlOnHit(rhythm::ttl::VisitRecency::StaleNewSession).count(), 0);
}

TEST(TtlPolicy, ClassifyVisitWithoutLastSeen) {
    EXPECT_EQ(rhythm::ttl::ClassifyVisit(std::nullopt), rhythm::ttl::VisitRecency::StaleNewSession);
}

TEST(TtlPolicy, ClassifyVeryOldVisitIsStale) {
    using Clock = std::chrono::system_clock;
    const auto ancient = Clock::now() - (rhythm::ttl::kStaleAfterNoVisit + std::chrono::hours{1});
    EXPECT_EQ(rhythm::ttl::ClassifyVisit(ancient), rhythm::ttl::VisitRecency::StaleNewSession);
}
