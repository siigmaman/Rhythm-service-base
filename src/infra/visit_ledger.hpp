#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace rhythm::infra {

/// Время последнего GET /recommend для ttl_policy. В памяти — VisitLedger; с Redis — RedisRhythmData.
class IVisitLedger {
   public:
    virtual ~IVisitLedger() = default;

    virtual std::optional<std::chrono::system_clock::time_point> LastSeenUtc(const std::string& user_id) = 0;

    /// Вызывается на каждый запрос GET /recommend.
    virtual void MarkSeenUtc(const std::string& user_id) = 0;
};

/// Процесс-локальный журнал (до включения Redis на всех инстансах).
class VisitLedger final : public IVisitLedger {
   public:
    std::optional<std::chrono::system_clock::time_point> LastSeenUtc(const std::string& user_id) override;

    void MarkSeenUtc(const std::string& user_id) override;

   private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_seen_;
};

}  // namespace rhythm::infra
