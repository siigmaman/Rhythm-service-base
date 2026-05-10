#pragma once

#include <cstdint>
#include <string>

namespace rhythm::infra::redis_keys {

// Имена ключей; {...} — hash-tag для Redis Cluster.

inline std::string FeedKey(const std::string& user_id) { return "{rhythm:" + user_id + "}:feed"; }

inline std::string VisitLastKey(const std::string& user_id) { return "{rhythm:" + user_id + "}:visit_last"; }

inline std::string GptRankKey(const std::string& user_id) { return "{rhythm:" + user_id + "}:gpt_rank"; }

inline std::string SimilarKey(std::int64_t anchor_product_id) {
    return "{rhythm:similar}:" + std::to_string(anchor_product_id);
}

inline std::string PopularGlobalKey() { return "{rhythm}:popular_global"; }

inline std::string IdempotencyKey(const std::string& token) { return "{rhythm:idempotency}:" + token; }

}  // namespace rhythm::infra::redis_keys
