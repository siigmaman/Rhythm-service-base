#include "ttl_policy.hpp"

#include <chrono>

namespace rhythm::ttl {

namespace {

using Clock = std::chrono::system_clock;

bool IsSameUtcDay(Clock::time_point a, Clock::time_point b) {
    const auto da = std::chrono::floor<std::chrono::days>(a);
    const auto db = std::chrono::floor<std::chrono::days>(b);
    return da == db;
}

bool IsPreviousUtcDay(Clock::time_point now, Clock::time_point prev) {
    const auto d_now = std::chrono::floor<std::chrono::days>(now);
    const auto d_prev = std::chrono::floor<std::chrono::days>(prev);
    return (d_now - d_prev) == std::chrono::days{1};
}

}  // namespace

VisitRecency ClassifyVisit(const std::optional<Clock::time_point>& last_seen) {
    if (!last_seen) return VisitRecency::StaleNewSession;
    const auto now = Clock::now();

    if (ShouldInvalidateFeed(last_seen)) {
        return VisitRecency::StaleNewSession;
    }
    if (IsSameUtcDay(now, *last_seen)) return VisitRecency::Today;
    if (IsPreviousUtcDay(now, *last_seen)) return VisitRecency::Yesterday;
    return VisitRecency::StaleNewSession;
}

bool ShouldInvalidateFeed(const std::optional<Clock::time_point>& last_seen) {
    if (!last_seen) return true;
    const auto now = Clock::now();
    if (now - *last_seen >= kStaleAfterNoVisit) {
        return true;
    }
    return false;
}

std::chrono::seconds ExtendFeedTtlOnHit(VisitRecency recency) {
    switch (recency) {
        case VisitRecency::Today:
            return std::chrono::duration_cast<std::chrono::seconds>(kExtendIfVisitedToday);
        case VisitRecency::Yesterday:
            return std::chrono::duration_cast<std::chrono::seconds>(kExtendIfVisitedYesterday);
        case VisitRecency::StaleNewSession:
            return std::chrono::seconds{0};
    }
    return std::chrono::seconds{0};
}

}  // namespace rhythm::ttl
