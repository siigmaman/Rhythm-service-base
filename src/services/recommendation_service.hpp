#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../infra/ports.hpp"
#include "../infra/visit_ledger.hpp"

namespace recommendation_service {

class RecommendationService final {
   public:
    RecommendationService(rhythm::infra::IRecommendationCachePort& cache,
                          rhythm::infra::ISideRecommendationCache& side_cache, rhythm::infra::IGptRecommendPort& gpt,
                          rhythm::infra::IUserBehaviorStorePort& behavior, rhythm::infra::IVisitLedger& visits);

    std::vector<std::int64_t> GetRecommendations(const std::string& user_id);

   private:
    struct RecommendationSources final {
        std::vector<std::int64_t> history;
        std::vector<std::int64_t> popular;
        std::vector<std::int64_t> gpt_ranked;
        std::vector<std::int64_t> similar;
    };

    RecommendationSources FetchSources(const std::string& user_id);

    static std::vector<std::int64_t> FanOutMerge(const RecommendationSources& sources);

    static std::vector<std::int64_t> TakeTop(const std::vector<std::int64_t>& ranked, std::size_t limit);

    rhythm::infra::IRecommendationCachePort& cache_;
    rhythm::infra::ISideRecommendationCache& side_cache_;
    rhythm::infra::IGptRecommendPort& gpt_;
    rhythm::infra::IUserBehaviorStorePort& behavior_;
    rhythm::infra::IVisitLedger& visits_;
};

}  // namespace recommendation_service
