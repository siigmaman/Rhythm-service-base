#pragma once

#include <chrono>
#include <optional>

namespace rhythm::ttl {

inline constexpr auto kFeedListSize = std::size_t{50};
inline constexpr auto kGptRecommendationIds = std::size_t{100};

// Кэши (целевые TTL)
inline constexpr auto kFeedKeyTtl = std::chrono::seconds{96 * 60};     // ~1.6 ч
inline constexpr auto kGptRankingTtl = std::chrono::seconds{30 * 60};  // 30 мин
inline constexpr auto kSimilarProductsTtl = std::chrono::hours{24};
inline constexpr auto kPopularProductsTtl = std::chrono::hours{1};

// Продление TTL при hit (зависит от «давности» визита)
inline constexpr auto kExtendIfVisitedToday = std::chrono::hours{1};
inline constexpr auto kExtendIfVisitedYesterday = std::chrono::hours{6};
inline constexpr auto kStaleAfterNoVisit = std::chrono::hours{24};

/// Окно удержания токена индемпотентности POST /action (клиент может безопасно повторить запрос).
inline constexpr auto kIdempotencyHold = std::chrono::hours{48};

enum class VisitRecency {
    Today,
    Yesterday,
    StaleNewSession,
};

// Классификация по времени последнего GET /recommend (UTC-день / вчера / давно).
VisitRecency ClassifyVisit(const std::optional<std::chrono::system_clock::time_point>& last_seen);

/// Сколько продлить ключ ленты при успешном cache hit.
std::chrono::seconds ExtendFeedTtlOnHit(VisitRecency recency);

/// Должны ли сбросить кэш ленты (нет визита > суток).
bool ShouldInvalidateFeed(const std::optional<std::chrono::system_clock::time_point>& last_seen);

}  // namespace rhythm::ttl
