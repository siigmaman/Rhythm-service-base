#include "visit_ledger.hpp"

namespace rhythm::infra {

std::optional<std::chrono::system_clock::time_point> VisitLedger::LastSeenUtc(const std::string& user_id) {
    std::lock_guard lock(mutex_);
    auto it = last_seen_.find(user_id);
    if (it == last_seen_.end()) return std::nullopt;
    return it->second;
}

void VisitLedger::MarkSeenUtc(const std::string& user_id) {
    std::lock_guard lock(mutex_);
    last_seen_[user_id] = std::chrono::system_clock::now();
}

}  // namespace rhythm::infra
