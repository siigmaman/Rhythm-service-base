#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "ports.hpp"
#include "visit_ledger.hpp"

namespace rhythm::infra {

/// Лента, side-кэши, идемпотентность и журнал визитов в Redis (ключи — redis_keyspace.hpp).
class RedisRhythmData final : public userver::components::ComponentBase,
                              public IRecommendationCachePort,
                              public ISideRecommendationCache,
                              public IIdempotencyPort,
                              public IVisitLedger {
   public:
    static constexpr std::string_view kName = "rhythm-redis-data";

    RedisRhythmData(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    void OnAllComponentsLoaded() override;

    std::optional<FeedCacheEntry> TryGetFeed(const std::string& user_id) override;
    void PutFeed(const std::string& user_id, const std::vector<std::int64_t>& ids, std::chrono::seconds ttl) override;
    void TouchFeed(const std::string& user_id, std::chrono::seconds extend_by) override;
    void DropFeed(const std::string& user_id) override;

    std::optional<std::vector<std::int64_t>> TryGetGptRanked(const std::string& user_id) override;
    void PutGptRanked(const std::string& user_id, const std::vector<std::int64_t>& ids,
                      std::chrono::seconds ttl) override;

    std::optional<std::vector<std::int64_t>> TryGetSimilar(std::int64_t anchor_product_id) override;
    void PutSimilar(std::int64_t anchor_product_id, const std::vector<std::int64_t>& ids,
                    std::chrono::seconds ttl) override;

    std::optional<std::vector<std::int64_t>> TryGetPopularGlobal() override;
    void PutPopularGlobal(const std::vector<std::int64_t>& ids, std::chrono::seconds ttl) override;

    void InvalidateUserDerived(const std::string& user_id) override;

    /// Вызов из воркера после INSERT в user_actions: лента, GPT-кэш пользователя, visit_last.
    void InvalidateAfterUserAction(const std::string& user_id);

    bool TryReserve(const std::string& token, std::chrono::seconds ttl) override;

    std::optional<std::chrono::system_clock::time_point> LastSeenUtc(const std::string& user_id) override;
    void MarkSeenUtc(const std::string& user_id) override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

   private:
    std::shared_ptr<userver::storages::redis::Client> client_;
    userver::storages::redis::CommandControl cc_;
    std::chrono::seconds visit_last_ttl_;
};

}  // namespace rhythm::infra
