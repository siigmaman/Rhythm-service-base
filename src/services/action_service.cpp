#include "action_service.hpp"

#include <stdexcept>

#include <userver/formats/json.hpp>

#include "../domain/ttl_policy.hpp"

namespace action_service {

namespace {

void EnsureNonEmpty(const std::string& field, const std::string& name) {
    if (field.empty()) {
        throw std::runtime_error("validation: empty field '" + name + "'");
    }
}

bool IsAllowedActionType(const std::string& t) {
    return t == "view" || t == "purchase" || t == "cart_add" || t == "like" || t == "save";
}

}  // namespace

ActionService::ActionService(rhythm::infra::IEventQueuePort& queue, rhythm::infra::IIdempotencyPort& idempotency)
    : queue_(queue), idempotency_(idempotency) {}

HandleActionResult ActionService::HandleAction(const ActionPayload& payload) {
    EnsureNonEmpty(payload.user_id, "user_id");
    EnsureNonEmpty(payload.action_type, "action_type");
    EnsureNonEmpty(payload.product_id, "product_id");
    EnsureNonEmpty(payload.idempotency_key, "idempotency_key");

    if (!IsAllowedActionType(payload.action_type)) {
        throw std::runtime_error("validation: unknown action_type");
    }

    const auto idem_ttl = std::chrono::duration_cast<std::chrono::seconds>(rhythm::ttl::kIdempotencyHold);
    if (!idempotency_.TryReserve(payload.idempotency_key, idem_ttl)) {
        return HandleActionResult::Duplicate;
    }

    userver::formats::json::ValueBuilder doc;
    doc["user_id"] = payload.user_id;
    doc["action_type"] = payload.action_type;
    doc["product_id"] = payload.product_id;
    doc["idempotency_key"] = payload.idempotency_key;

    queue_.PublishJson("user.action", userver::formats::json::ToString(doc.ExtractValue()));
    return HandleActionResult::Accepted;
}

}  // namespace action_service
