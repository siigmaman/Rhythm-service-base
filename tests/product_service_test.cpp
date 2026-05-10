#include <gtest/gtest.h>

#include <userver/formats/json.hpp>

#include "infra/in_memory_stubs.hpp"
#include "services/product_service.hpp"

TEST(ProductService, ReturnsJsonWhenFound) {
    rhythm::infra::StubProductCatalog catalog({{42, "sku", "Title"}});
    product_service::ProductService svc(catalog);
    auto j = svc.GetProductJson(42);
    ASSERT_TRUE(j.has_value());
    EXPECT_EQ((*j)["id"].As<std::int64_t>(), 42);
    EXPECT_EQ((*j)["sku"].As<std::string>(), "sku");
    EXPECT_EQ((*j)["title"].As<std::string>(), "Title");
}

TEST(ProductService, NulloptWhenMissing) {
    rhythm::infra::StubProductCatalog catalog({});
    product_service::ProductService svc(catalog);
    EXPECT_FALSE(svc.GetProductJson(1).has_value());
}
