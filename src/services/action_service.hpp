#pragma once

#include <string>

#include "../infra/ports.hpp"

namespace action_service {

struct ActionPayload final {
    std::string user_id;
    std::string action_type;  // view, purchase, cart_add, like, save
    std::string product_id;
    std::string idempotency_key;
};

enum class HandleActionResult {
    Accepted,
    Duplicate,
};

class ActionService final {
   public:
    ActionService(rhythm::infra::IEventQueuePort& queue, rhythm::infra::IIdempotencyPort& idempotency);

    /// Валидация, TryReserve, публикация; при дубликате ключа — без повторной отправки в очередь.
    HandleActionResult HandleAction(const ActionPayload& payload);

   private:
    rhythm::infra::IEventQueuePort& queue_;
    rhythm::infra::IIdempotencyPort& idempotency_;
};

}  // namespace action_service
