#include <gtest/gtest.h>

#include "infra/visit_ledger.hpp"

TEST(VisitLedger, LastSeenInitiallyEmpty) {
    rhythm::infra::VisitLedger ledger;
    EXPECT_FALSE(ledger.LastSeenUtc("u").has_value());
}

TEST(VisitLedger, MarkSeenThenLastSeenSet) {
    rhythm::infra::VisitLedger ledger;
    ledger.MarkSeenUtc("u");
    const auto t = ledger.LastSeenUtc("u");
    ASSERT_TRUE(t.has_value());
}
