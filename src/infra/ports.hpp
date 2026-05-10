#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rhythm::infra {

struct FeedCacheEntry {
    std::vector<std::int64_t> product_ids;
    std::chrono::seconds ttl_remaining{};
};

class IRecommendationCachePort {
   public:
    virtual ~IRecommendationCachePort() = default;

    virtual std::optional<FeedCacheEntry> TryGetFeed(const std::string& user_id) = 0;
    virtual void PutFeed(const std::string& user_id, const std::vector<std::int64_t>& ids,
                         std::chrono::seconds ttl) = 0;
    virtual void TouchFeed(const std::string& user_id, std::chrono::seconds extend_by) = 0;
    virtual void DropFeed(const std::string& user_id) = 0;
};

class IEventQueuePort {
   public:
    virtual ~IEventQueuePort() = default;
    virtual void PublishJson(const std::string& routing_key, const std::string& json_payload) = 0;
};

struct ProductRow {
    std::int64_t id{};
    std::string sku;
    std::string title;
};

class IProductCatalogPort {
   public:
    virtual ~IProductCatalogPort() = default;
    virtual std::optional<ProductRow> GetById(std::int64_t id) const = 0;
};

class IGptRecommendPort {
   public:
    virtual ~IGptRecommendPort() = default;
    virtual std::vector<std::int64_t> RankProductIds(const std::string& user_id, std::size_t limit) const = 0;
};

class IUserBehaviorStorePort {
   public:
    virtual ~IUserBehaviorStorePort() = default;
    virtual std::vector<std::int64_t> RecentProductIds(const std::string& user_id, std::size_t limit) const = 0;
    virtual std::vector<std::int64_t> PopularProductIds(std::size_t limit) const = 0;

    /// Соседи по category_id в каталоге; пусто — в RecommendationService берётся PopularProductIds.
    virtual std::vector<std::int64_t> CatalogNeighborProductIds(std::int64_t anchor_product_id,
                                                                std::size_t limit) const = 0;
};

/// Side-кэши рекомендаций (популярное, GPT, похожие); в Redis — разные ключи и TTL.
class ISideRecommendationCache {
   public:
    virtual ~ISideRecommendationCache() = default;

    virtual std::optional<std::vector<std::int64_t>> TryGetGptRanked(const std::string& user_id) = 0;
    virtual void PutGptRanked(const std::string& user_id, const std::vector<std::int64_t>& ids,
                              std::chrono::seconds ttl) = 0;

    virtual std::optional<std::vector<std::int64_t>> TryGetSimilar(std::int64_t anchor_product_id) = 0;
    virtual void PutSimilar(std::int64_t anchor_product_id, const std::vector<std::int64_t>& ids,
                            std::chrono::seconds ttl) = 0;

    virtual std::optional<std::vector<std::int64_t>> TryGetPopularGlobal() = 0;
    virtual void PutPopularGlobal(const std::vector<std::int64_t>& ids, std::chrono::seconds ttl) = 0;

    /// Сброс персональных кэшей после нового события (лента — отдельно через IRecommendationCachePort).
    virtual void InvalidateUserDerived(const std::string& user_id) = 0;
};

/// Дедупликация POST /action по Idempotency-Key.
class IIdempotencyPort {
   public:
    virtual ~IIdempotencyPort() = default;

    /// true — первый запрос с таким токеном, false — повтор в пределах TTL.
    virtual bool TryReserve(const std::string& token, std::chrono::seconds ttl) = 0;
};

}  // namespace rhythm::infra
