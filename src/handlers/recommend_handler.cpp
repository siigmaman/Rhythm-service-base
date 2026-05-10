#include "recommend_handler.hpp"

#include <userver/formats/common/type.hpp>
#include <userver/formats/json.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>

#include "../app/port_resolution.hpp"
#include "http_json_helpers.hpp"

namespace rhythm::handlers {

RecommendHandler::RecommendHandler(const userver::components::ComponentConfig& config,
                                   const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      service_(rhythm::app::RecommendationCache(context), rhythm::app::SideRecommendationCache(context),
               rhythm::app::GptRecommend(context), rhythm::app::BehaviorStore(context),
               rhythm::app::VisitLedgerPort(context)) {}

std::string RecommendHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                 userver::server::request::RequestContext&) const {
    auto& resp = request.GetHttpResponse();
    resp.SetContentType(userver::http::content_type::kApplicationJson);

    const auto user_id = request.GetArg("user_id");
    if (user_id.empty()) {
        resp.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return http_json::JsonError("missing_user_id");
    }

    auto items = service_.GetRecommendations(user_id);

    userver::formats::json::ValueBuilder obj;
    userver::formats::json::ValueBuilder arr{userver::formats::common::Type::kArray};
    for (const auto id : items) {
        arr.PushBack(id);
    }
    obj["items"] = arr;
    return userver::formats::json::ToString(obj.ExtractValue());
}

}  // namespace rhythm::handlers
