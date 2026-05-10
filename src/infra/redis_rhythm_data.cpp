#include "redis_rhythm_data.hpp"

#include <cstdint>
#include <stdexcept>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

#include "redis_keyspace.hpp"

namespace rhythm::infra {

namespace {

std::string IdsToJson(const std::vector<std::int64_t>& ids) {
    userver::formats::json::ValueBuilder arr{userver::formats::common::Type::kArray};
    for (const auto id : ids) {
        arr.PushBack(id);
    }
    return userver::formats::json::ToString(arr.ExtractValue());
}

bool JsonToIds(const std::string& raw, std::vector<std::int64_t>& out) {
    out.clear();
    userver::formats::json::Value json;
    try {
        json = userver::formats::json::FromString(raw);
    } catch (const std::exception&) {
        return false;
    }
    if (!json.IsArray()) return false;
    for (const auto& el : json) {
        if (el.IsInt64()) {
            out.push_back(el.As<std::int64_t>());
        } else if (el.IsUInt64()) {
            out.push_back(static_cast<std::int64_t>(el.As<std::uint64_t>()));
        } else {
            return false;
        }
    }
    return true;
}

}  // namespace

RedisRhythmData::RedisRhythmData(const userver::components::ComponentConfig& config,
                                 const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      client_(
          context.FindComponent<userver::components::Redis>(config["redis_component"].As<std::string>("rhythm-redis"))
              .GetClient(
                  config["redis_db"].As<std::string>("rhythm-cache"),
                  userver::storages::redis::RedisWaitConnected{userver::storages::redis::WaitConnectedMode::kNoWait,
                                                               false, std::chrono::milliseconds{500}})),
      visit_last_ttl_{std::chrono::seconds{config["visit_ledger_ttl"].As<int>(7776000)}} {}

void RedisRhythmData::OnAllComponentsLoaded() {
    userver::components::ComponentBase::OnAllComponentsLoaded();
    client_->WaitConnectedOnce(userver::storages::redis::RedisWaitConnected{
        userver::storages::redis::WaitConnectedMode::kMaster, false, std::chrono::milliseconds{8000}});
    LOG_INFO() << "rhythm-redis-data: redis client ready (cluster=" << client_->IsInClusterMode() << ")";
}

std::optional<FeedCacheEntry> RedisRhythmData::TryGetFeed(const std::string& user_id) {
    const auto key = redis_keys::FeedKey(user_id);
    const auto raw = client_->Get(key, cc_).Get();
    if (!raw) return std::nullopt;
    std::vector<std::int64_t> ids;
    if (!JsonToIds(*raw, ids)) {
        LOG_WARNING() << "rhythm-redis-data: corrupt feed json for " << key;
        client_->Del(key, cc_).Get();
        return std::nullopt;
    }
    const auto ttl_rep = client_->Ttl(key, cc_).Get();
    if (!ttl_rep.KeyExists()) return std::nullopt;
    if (!ttl_rep.KeyHasExpiration()) {
        return FeedCacheEntry{.product_ids = std::move(ids), .ttl_remaining = std::chrono::seconds{0}};
    }
    return FeedCacheEntry{.product_ids = std::move(ids), .ttl_remaining = ttl_rep.GetExpire()};
}

void RedisRhythmData::PutFeed(const std::string& user_id, const std::vector<std::int64_t>& ids,
                              std::chrono::seconds ttl) {
    const auto key = redis_keys::FeedKey(user_id);
    client_->Setex(key, ttl, IdsToJson(ids), cc_).Get();
}

void RedisRhythmData::TouchFeed(const std::string& user_id, std::chrono::seconds extend_by) {
    if (extend_by.count() <= 0) return;
    const auto key = redis_keys::FeedKey(user_id);
    const auto ttl_rep = client_->Ttl(key, cc_).Get();
    if (!ttl_rep.KeyExists() || !ttl_rep.KeyHasExpiration()) return;
    const auto new_ttl = ttl_rep.GetExpire() + extend_by;
    client_->Expire(key, new_ttl, cc_).Get();
}

void RedisRhythmData::DropFeed(const std::string& user_id) { client_->Del(redis_keys::FeedKey(user_id), cc_).Get(); }

std::optional<std::vector<std::int64_t>> RedisRhythmData::TryGetGptRanked(const std::string& user_id) {
    const auto key = redis_keys::GptRankKey(user_id);
    const auto raw = client_->Get(key, cc_).Get();
    if (!raw) return std::nullopt;
    std::vector<std::int64_t> ids;
    if (!JsonToIds(*raw, ids)) {
        client_->Del(key, cc_).Get();
        return std::nullopt;
    }
    return ids;
}

void RedisRhythmData::PutGptRanked(const std::string& user_id, const std::vector<std::int64_t>& ids,
                                   std::chrono::seconds ttl) {
    client_->Setex(redis_keys::GptRankKey(user_id), ttl, IdsToJson(ids), cc_).Get();
}

std::optional<std::vector<std::int64_t>> RedisRhythmData::TryGetSimilar(std::int64_t anchor_product_id) {
    const auto key = redis_keys::SimilarKey(anchor_product_id);
    const auto raw = client_->Get(key, cc_).Get();
    if (!raw) return std::nullopt;
    std::vector<std::int64_t> ids;
    if (!JsonToIds(*raw, ids)) {
        client_->Del(key, cc_).Get();
        return std::nullopt;
    }
    return ids;
}

void RedisRhythmData::PutSimilar(std::int64_t anchor_product_id, const std::vector<std::int64_t>& ids,
                                 std::chrono::seconds ttl) {
    client_->Setex(redis_keys::SimilarKey(anchor_product_id), ttl, IdsToJson(ids), cc_).Get();
}

std::optional<std::vector<std::int64_t>> RedisRhythmData::TryGetPopularGlobal() {
    const auto key = redis_keys::PopularGlobalKey();
    const auto raw = client_->Get(key, cc_).Get();
    if (!raw) return std::nullopt;
    std::vector<std::int64_t> ids;
    if (!JsonToIds(*raw, ids)) {
        client_->Del(key, cc_).Get();
        return std::nullopt;
    }
    return ids;
}

void RedisRhythmData::PutPopularGlobal(const std::vector<std::int64_t>& ids, std::chrono::seconds ttl) {
    client_->Setex(redis_keys::PopularGlobalKey(), ttl, IdsToJson(ids), cc_).Get();
}

void RedisRhythmData::InvalidateUserDerived(const std::string& user_id) {
    client_->Del(redis_keys::GptRankKey(user_id), cc_).Get();
}

void RedisRhythmData::InvalidateAfterUserAction(const std::string& user_id) {
    DropFeed(user_id);
    InvalidateUserDerived(user_id);
    client_->Del(redis_keys::VisitLastKey(user_id), cc_).Get();
}

bool RedisRhythmData::TryReserve(const std::string& token, std::chrono::seconds ttl) {
    const auto key = redis_keys::IdempotencyKey(token);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ttl);
    return client_->SetIfNotExist(key, "1", ms, cc_).Get();
}

std::optional<std::chrono::system_clock::time_point> RedisRhythmData::LastSeenUtc(const std::string& user_id) {
    const auto key = redis_keys::VisitLastKey(user_id);
    const auto raw = client_->Get(key, cc_).Get();
    if (!raw || raw->empty()) return std::nullopt;
    try {
        const auto ms_count = std::stoll(*raw);
        if (ms_count < 0) {
            client_->Del(key, cc_).Get();
            return std::nullopt;
        }
        return std::chrono::system_clock::time_point{std::chrono::milliseconds{ms_count}};
    } catch (const std::exception&) {
        client_->Del(key, cc_).Get();
        return std::nullopt;
    }
}

void RedisRhythmData::MarkSeenUtc(const std::string& user_id) {
    const auto key = redis_keys::VisitLastKey(user_id);
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    client_->Setex(key, visit_last_ttl_, std::to_string(ms), cc_).Get();
}

userver::yaml_config::Schema RedisRhythmData::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Redis-backed feed, side caches, idempotency, visit ledger
additionalProperties: false
properties:
    redis_component:
        type: string
        description: Name of components::Redis in static config
        defaultDescription: rhythm-redis
    redis_db:
        type: string
        description: Group db name for Redis::GetClient (see redis.groups[].db)
        defaultDescription: rhythm-cache
    visit_ledger_ttl:
        type: integer
        description: TTL (seconds) for per-user visit_last key in Redis (rolling refresh on each GET /recommend)
        defaultDescription: '7776000'
)");
}

}  // namespace rhythm::infra
