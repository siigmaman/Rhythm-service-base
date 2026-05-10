#pragma once

#include <optional>
#include <string>

#include <userver/formats/json/value.hpp>

#include "../infra/ports.hpp"

namespace product_service {

class ProductService final {
   public:
    explicit ProductService(rhythm::infra::IProductCatalogPort& catalog);

    /// JSON для ответа или nullopt если нет товара.
    std::optional<userver::formats::json::Value> GetProductJson(std::int64_t id) const;

   private:
    rhythm::infra::IProductCatalogPort& catalog_;
};

}  // namespace product_service
