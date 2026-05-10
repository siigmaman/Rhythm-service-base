#include "recommendation_service.hpp"

#include <algorithm>
#include <unordered_map>

#include "../domain/ttl_policy.hpp"

namespace recommendation_service {

namespace {

constexpr int WeightHistory = 3;
constexpr int WeightPopular = 2;
constexpr int WeightGpt = 4;
constexpr int WeightSimilar = 3;

}  // namespace

RecommendationService::RecommendationService(rhythm::infra::IRecommendationCachePort& cache,
                                             rhythm::infra::ISideRecommendationCache& side_cache,
                                             rhythm::infra::IGptRecommendPort& gpt,
                                             rhythm::infra::IUserBehaviorStorePort& behavior,
                                             rhythm::infra::IVisitLedger& visits)
    : cache_(cache), side_cache_(side_cache), gpt_(gpt), behavior_(behavior), visits_(visits) {}

RecommendationService::RecommendationSources RecommendationService::FetchSources(const std::string& user_id) {
    RecommendationSources sources;
    sources.history = behavior_.RecentProductIds(user_id, 120);

    if (const auto cached = side_cache_.TryGetPopularGlobal()) {
        sources.popular = *cached;
    } else {
        sources.popular = behavior_.PopularProductIds(120);
        side_cache_.PutPopularGlobal(sources.popular, rhythm::ttl::kPopularProductsTtl);
    }

    if (const auto cached = side_cache_.TryGetGptRanked(user_id)) {
        sources.gpt_ranked = *cached;
    } else {
        sources.gpt_ranked = gpt_.RankProductIds(user_id, rhythm::ttl::kGptRecommendationIds);
        side_cache_.PutGptRanked(user_id, sources.gpt_ranked, rhythm::ttl::kGptRankingTtl);
    }

    std::int64_t similar_anchor = 0;
    if (!sources.history.empty()) {
        similar_anchor = sources.history.front();
    } else if (!sources.popular.empty()) {
        similar_anchor = sources.popular.front();
    }
    if (similar_anchor != 0) {
        if (const auto cached = side_cache_.TryGetSimilar(similar_anchor)) {
            sources.similar = *cached;
        } else {
            auto similar = behavior_.CatalogNeighborProductIds(similar_anchor, 60);
            if (similar.empty()) {
                similar = behavior_.PopularProductIds(60);
            }
            sources.similar = std::move(similar);
            side_cache_.PutSimilar(similar_anchor, sources.similar, rhythm::ttl::kSimilarProductsTtl);
        }
    }
    return sources;
}

std::vector<std::int64_t> RecommendationService::FanOutMerge(const RecommendationSources& sources) {
    std::unordered_map<std::int64_t, int> score;
    auto add = [&](const std::vector<std::int64_t>& vec, int w) {
        for (const auto id : vec) {
            score[id] += w;
        }
    };

    add(sources.history, WeightHistory);
    add(sources.popular, WeightPopular);
    add(sources.gpt_ranked, WeightGpt);
    add(sources.similar, WeightSimilar);

    std::vector<std::pair<std::int64_t, int>> items;
    items.reserve(score.size());
    for (auto& kv : score) {
        items.push_back(kv);
    }

    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    std::vector<std::int64_t> out;
    out.reserve(items.size());
    for (const auto& e : items) {
        out.push_back(e.first);
    }
    return out;
}

std::vector<std::int64_t> RecommendationService::TakeTop(const std::vector<std::int64_t>& ranked, std::size_t limit) {
    if (ranked.size() <= limit) return ranked;
    return std::vector<std::int64_t>(ranked.begin(), ranked.begin() + static_cast<std::ptrdiff_t>(limit));
}

std::vector<std::int64_t> RecommendationService::GetRecommendations(const std::string& user_id) {
    const auto prev_seen = visits_.LastSeenUtc(user_id);

    if (rhythm::ttl::ShouldInvalidateFeed(prev_seen)) {
        cache_.DropFeed(user_id);
    }

    visits_.MarkSeenUtc(user_id);

    auto cached = cache_.TryGetFeed(user_id);
    if (cached) {
        const auto recency = rhythm::ttl::ClassifyVisit(prev_seen);
        cache_.TouchFeed(user_id, rhythm::ttl::ExtendFeedTtlOnHit(recency));
        return TakeTop(cached->product_ids, rhythm::ttl::kFeedListSize);
    }

    auto sources = FetchSources(user_id);
    auto merged = FanOutMerge(sources);
    const auto top_ranked = TakeTop(merged, rhythm::ttl::kGptRecommendationIds);
    cache_.PutFeed(user_id, top_ranked, rhythm::ttl::kFeedKeyTtl);
    return TakeTop(top_ranked, rhythm::ttl::kFeedListSize);
}

}  // namespace recommendation_service
