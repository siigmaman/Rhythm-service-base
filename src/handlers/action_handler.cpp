#include "action_handler.hpp"

#include <string>

#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>

#include "../app/port_resolution.hpp"
#include "http_json_helpers.hpp"

namespace rhythm::handlers {

ActionHandler::ActionHandler(const userver::components::ComponentConfig& config,
                             const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), service_(rhythm::app::EventQueue(context), rhythm::app::Idempotency(context)) {}

std::string ActionHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    auto& resp = request.GetHttpResponse();
    resp.SetContentType(userver::http::content_type::kApplicationJson);

    const auto idempotency_key = http_json::TrimCopy(request.GetHeader("Idempotency-Key"));
    if (idempotency_key.empty()) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("missing_idempotency_key");
    }
    if (idempotency_key.size() > 256) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("idempotency_key_too_long");
    }

    const auto body = request.RequestBody();
    if (body.empty()) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("empty_body");
    }

    userver::formats::json::Value json;
    try {
        json = userver::formats::json::FromString(body);
    } catch (const std::exception& ex) {
        LOG_WARNING() << "action json parse failed: " << ex.what();
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("invalid_json");
    }

    try {
        action_service::ActionPayload payload{
            http_json::JsonFieldAsString(json, "user_id"),
            http_json::JsonFieldAsString(json, "action_type"),
            http_json::JsonFieldAsString(json, "product_id"),
            idempotency_key,
        };
        const auto r = service_.HandleAction(payload);
        resp.SetStatus(userver::server::http::HttpStatus::kAccepted);
        return http_json::JsonActionAccepted(r == action_service::HandleActionResult::Duplicate);
    } catch (const std::exception& ex) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError(ex.what());
    }
}

}  // namespace rhythm::handlers
