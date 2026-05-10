#include <algorithm>

#include <gtest/gtest.h>

#include "domain/ttl_policy.hpp"
#include "infra/in_memory_stubs.hpp"
#include "infra/visit_ledger.hpp"
#include "services/recommendation_service.hpp"

namespace {

class MockBehavior final : public rhythm::infra::IUserBehaviorStorePort {
   public:
    std::vector<std::int64_t> recent;
    std::vector<std::int64_t> popular;
    std::vector<std::int64_t> neighbors;

    std::vector<std::int64_t> RecentProductIds(const std::string&, std::size_t limit) const override {
        auto v = recent;
        if (v.size() > limit) v.resize(limit);
        return v;
    }
    std::vector<std::int64_t> PopularProductIds(std::size_t limit) const override {
        auto v = popular;
        if (v.size() > limit) v.resize(limit);
        return v;
    }
    std::vector<std::int64_t> CatalogNeighborProductIds(std::int64_t, std::size_t limit) const override {
        auto v = neighbors;
        if (v.size() > limit) v.resize(limit);
        return v;
    }
};

class MockGpt final : public rhythm::infra::IGptRecommendPort {
   public:
    std::vector<std::int64_t> ranks;
    std::vector<std::int64_t> RankProductIds(const std::string&, std::size_t limit) const override {
        auto v = ranks;
        if (v.size() > limit) v.resize(limit);
        return v;
    }
};

}  // namespace

TEST(RecommendationService, FanOutMergeOrderingByWeightedScore) {
    rhythm::infra::InMemoryRhythmData cache;
    MockGpt gpt;
    MockBehavior behavior;
    behavior.recent = {1};
    behavior.popular = {2};
    behavior.neighbors = {10};
    gpt.ranks = {1, 3};

    rhythm::infra::VisitLedger visits;
    recommendation_service::RecommendationService svc(cache, cache, gpt, behavior, visits);

    const auto out = svc.GetRecommendations("merge_user");
    ASSERT_FALSE(out.empty());
    // id 1: history(3) + gpt(4) = 7 — максимум
    EXPECT_EQ(out.front(), 1);
    // id 3: только gpt(4); id 10: similar(3); id 2: popular(2)
    ASSERT_GE(out.size(), 4u);
    auto pos = [&](std::int64_t id) -> std::size_t {
        return static_cast<std::size_t>(std::find(out.begin(), out.end(), id) - out.begin());
    };
    EXPECT_LT(pos(1), pos(3));
    EXPECT_LT(pos(3), pos(10));
    EXPECT_LT(pos(10), pos(2));
}

TEST(RecommendationService, SecondRequestUsesFeedCache) {
    rhythm::infra::InMemoryRhythmData cache;
    rhythm::infra::StubGptRecommend gpt;
    rhythm::infra::StubBehaviorStore behavior;
    rhythm::infra::VisitLedger visits;

    recommendation_service::RecommendationService svc(cache, cache, gpt, behavior, visits);

    const auto a = svc.GetRecommendations("cache_user");
    const auto b = svc.GetRecommendations("cache_user");
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.size(), rhythm::ttl::kFeedListSize);
}
