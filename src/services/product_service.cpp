#include "product_service.hpp"

#include <userver/formats/json.hpp>

namespace product_service {

ProductService::ProductService(rhythm::infra::IProductCatalogPort& catalog) : catalog_(catalog) {}

std::optional<userver::formats::json::Value> ProductService::GetProductJson(std::int64_t id) const {
    auto row = catalog_.GetById(id);
    if (!row) return std::nullopt;

    userver::formats::json::ValueBuilder b;
    b["id"] = row->id;
    b["sku"] = row->sku;
    b["title"] = row->title;
    return b.ExtractValue();
}

}  // namespace product_service
