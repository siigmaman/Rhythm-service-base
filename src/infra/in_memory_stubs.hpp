#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ports.hpp"

namespace rhythm::infra {

/// Лента, side-кэши и идемпотентность в памяти процесса (один mutex).
class InMemoryRhythmData final : public IRecommendationCachePort,
                                 public ISideRecommendationCache,
                                 public IIdempotencyPort {
   public:
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

    bool TryReserve(const std::string& token, std::chrono::seconds ttl) override;

   private:
    struct TimedIds final {
        std::vector<std::int64_t> ids;
        std::chrono::steady_clock::time_point expires_at{};
    };

    struct FeedCell final {
        std::vector<std::int64_t> ids;
        std::chrono::steady_clock::time_point expires_at{};
    };

    std::mutex mutex_;
    std::unordered_map<std::string, FeedCell> feed_;
    std::unordered_map<std::string, TimedIds> gpt_;
    std::unordered_map<std::int64_t, TimedIds> similar_;
    TimedIds popular_{};
    bool popular_inited_{false};
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> idempotency_;
};

/// Пишет тело сообщения в лог вместо брокера.
class LoggingEventQueue final : public IEventQueuePort {
   public:
    void PublishJson(const std::string& routing_key, const std::string& json_payload) override;
};

class StubProductCatalog final : public IProductCatalogPort {
   public:
    explicit StubProductCatalog(std::initializer_list<ProductRow> rows);

    std::optional<ProductRow> GetById(std::int64_t id) const override;

   private:
    std::unordered_map<std::int64_t, ProductRow> by_id_;
};

class StubGptRecommend final : public IGptRecommendPort {
   public:
    std::vector<std::int64_t> RankProductIds(const std::string& user_id, std::size_t limit) const override;
};

class StubBehaviorStore final : public IUserBehaviorStorePort {
   public:
    std::vector<std::int64_t> RecentProductIds(const std::string& user_id, std::size_t limit) const override;
    std::vector<std::int64_t> PopularProductIds(std::size_t limit) const override;
    std::vector<std::int64_t> CatalogNeighborProductIds(std::int64_t anchor_product_id,
                                                        std::size_t limit) const override;
};

}  // namespace rhythm::infra
