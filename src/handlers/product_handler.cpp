#include "product_handler.hpp"

#include <cstdint>

#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>

#include "../app/port_resolution.hpp"
#include "http_json_helpers.hpp"

namespace rhythm::handlers {

ProductHandler::ProductHandler(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), service_(rhythm::app::ProductCatalog(context)) {}

std::string ProductHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext&) const {
    auto& resp = request.GetHttpResponse();
    resp.SetContentType(userver::http::content_type::kApplicationJson);

    const auto id_raw = request.GetArg("id");
    if (id_raw.empty()) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("missing_query_id");
    }

    std::int64_t id{};
    try {
        id = std::stoll(id_raw);
    } catch (const std::exception&) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("invalid_id");
    }

    auto doc = service_.GetProductJson(id);
    if (!doc) {
        resp.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return http_json::JsonError("not_found");
    }

    resp.SetStatus(userver::server::http::HttpStatus::kOk);
    return userver::formats::json::ToString(*doc);
}

}  // namespace rhythm::handlers
