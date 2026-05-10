#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include "../services/product_service.hpp"

namespace rhythm::handlers {

/// GET /product?id= — карточка из каталога (Postgres при RHYTHM_WITH_POSTGRES, иначе заглушка).
class ProductHandler final : public userver::server::handlers::HttpHandlerBase {
   public:
    static constexpr std::string_view kName = "handler-product";

    ProductHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext& context) const override;

   private:
    mutable product_service::ProductService service_;
};

}  // namespace rhythm::handlers
