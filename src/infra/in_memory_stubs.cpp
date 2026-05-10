#include "in_memory_stubs.hpp"

#include <userver/logging/log.hpp>

namespace rhythm::infra {

namespace {

using Clock = std::chrono::steady_clock;

bool IsExpired(Clock::time_point expires_at) { return expires_at <= Clock::now(); }

}  // namespace

std::optional<FeedCacheEntry> InMemoryRhythmData::TryGetFeed(const std::string& user_id) {
    const auto now = Clock::now();
    std::lock_guard lock(mutex_);
    auto it = feed_.find(user_id);
    if (it == feed_.end()) return std::nullopt;
    if (it->second.expires_at <= now) {
        feed_.erase(it);
        return std::nullopt;
    }
    const auto ttl_left = std::chrono::duration_cast<std::chrono::seconds>(it->second.expires_at - now);
    return FeedCacheEntry{.product_ids = it->second.ids, .ttl_remaining = ttl_left};
}

void InMemoryRhythmData::PutFeed(const std::string& user_id, const std::vector<std::int64_t>& ids,
                                 std::chrono::seconds ttl) {
    std::lock_guard lock(mutex_);
    auto& cell = feed_[user_id];
    cell.ids = ids;
    cell.expires_at = Clock::now() + ttl;
}

void InMemoryRhythmData::TouchFeed(const std::string& user_id, std::chrono::seconds extend_by) {
    if (extend_by.count() <= 0) return;
    std::lock_guard lock(mutex_);
    auto it = feed_.find(user_id);
    if (it == feed_.end()) return;
    const auto now = Clock::now();
    if (it->second.expires_at <= now) {
        feed_.erase(it);
        return;
    }
    it->second.expires_at += extend_by;
}

void InMemoryRhythmData::DropFeed(const std::string& user_id) {
    std::lock_guard lock(mutex_);
    feed_.erase(user_id);
}

std::optional<std::vector<std::int64_t>> InMemoryRhythmData::TryGetGptRanked(const std::string& user_id) {
    std::lock_guard lock(mutex_);
    auto it = gpt_.find(user_id);
    if (it == gpt_.end()) return std::nullopt;
    if (IsExpired(it->second.expires_at)) {
        gpt_.erase(it);
        return std::nullopt;
    }
    return it->second.ids;
}

void InMemoryRhythmData::PutGptRanked(const std::string& user_id, const std::vector<std::int64_t>& ids,
                                      std::chrono::seconds ttl) {
    std::lock_guard lock(mutex_);
    auto& cell = gpt_[user_id];
    cell.ids = ids;
    cell.expires_at = Clock::now() + ttl;
}

std::optional<std::vector<std::int64_t>> InMemoryRhythmData::TryGetSimilar(std::int64_t anchor_product_id) {
    std::lock_guard lock(mutex_);
    auto it = similar_.find(anchor_product_id);
    if (it == similar_.end()) return std::nullopt;
    if (IsExpired(it->second.expires_at)) {
        similar_.erase(it);
        return std::nullopt;
    }
    return it->second.ids;
}

void InMemoryRhythmData::PutSimilar(std::int64_t anchor_product_id, const std::vector<std::int64_t>& ids,
                                    std::chrono::seconds ttl) {
    std::lock_guard lock(mutex_);
    auto& cell = similar_[anchor_product_id];
    cell.ids = ids;
    cell.expires_at = Clock::now() + ttl;
}

std::optional<std::vector<std::int64_t>> InMemoryRhythmData::TryGetPopularGlobal() {
    std::lock_guard lock(mutex_);
    if (!popular_inited_) return std::nullopt;
    if (IsExpired(popular_.expires_at)) {
        popular_inited_ = false;
        return std::nullopt;
    }
    return popular_.ids;
}

void InMemoryRhythmData::PutPopularGlobal(const std::vector<std::int64_t>& ids, std::chrono::seconds ttl) {
    std::lock_guard lock(mutex_);
    popular_.ids = ids;
    popular_.expires_at = Clock::now() + ttl;
    popular_inited_ = true;
}

void InMemoryRhythmData::InvalidateUserDerived(const std::string& user_id) {
    std::lock_guard lock(mutex_);
    gpt_.erase(user_id);
}

bool InMemoryRhythmData::TryReserve(const std::string& token, std::chrono::seconds ttl) {
    if (token.empty()) return false;
    const auto now = Clock::now();
    const auto until = now + ttl;
    std::lock_guard lock(mutex_);
    auto it = idempotency_.find(token);
    if (it != idempotency_.end() && it->second > now) {
        return false;
    }
    idempotency_[token] = until;
    return true;
}

void LoggingEventQueue::PublishJson(const std::string& routing_key, const std::string& json_payload) {
    LOG_INFO() << "queue.publish rk=" << routing_key << " body=" << json_payload;
}

StubProductCatalog::StubProductCatalog(std::initializer_list<ProductRow> rows) {
    for (const auto& r : rows) {
        by_id_[r.id] = r;
    }
}

std::optional<ProductRow> StubProductCatalog::GetById(std::int64_t id) const {
    auto it = by_id_.find(id);
    if (it == by_id_.end()) return std::nullopt;
    return it->second;
}

std::vector<std::int64_t> StubGptRecommend::RankProductIds(const std::string&, std::size_t limit) const {
    std::vector<std::int64_t> out;
    const auto cap = limit == 0 ? std::size_t{10} : limit;
    out.reserve(cap);
    for (std::size_t i = 0; i < cap; ++i) {
        out.push_back(static_cast<std::int64_t>(9000 + static_cast<std::int64_t>(i)));
    }
    return out;
}

std::vector<std::int64_t> StubBehaviorStore::RecentProductIds(const std::string& /*user_id*/, std::size_t limit) const {
    std::vector<std::int64_t> v{101, 202, 303};
    if (limit < v.size()) v.resize(limit);
    return v;
}

std::vector<std::int64_t> StubBehaviorStore::PopularProductIds(std::size_t limit) const {
    std::vector<std::int64_t> v{10, 20, 30, 40, 50};
    if (limit < v.size()) v.resize(limit);
    return v;
}

std::vector<std::int64_t> StubBehaviorStore::CatalogNeighborProductIds(std::int64_t, std::size_t limit) const {
    return PopularProductIds(limit);
}

}  // namespace rhythm::infra
