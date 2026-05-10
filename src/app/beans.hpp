#pragma once

#include "../infra/in_memory_stubs.hpp"
#include "../infra/visit_ledger.hpp"

namespace rhythm::app {

// Один набор заглушек на процесс, если бинарник без RHYTHM_WITH_*.
struct RhythmBeans final {
    rhythm::infra::InMemoryRhythmData rhythm_data{};
    rhythm::infra::LoggingEventQueue event_queue{};
    rhythm::infra::StubProductCatalog product_catalog{{{1, "SKU-1", "Demo kettle"}, {2, "SKU-2", "Demo phone"}}};
    rhythm::infra::StubGptRecommend gpt{};
    rhythm::infra::StubBehaviorStore behavior_store{};
    rhythm::infra::VisitLedger visit_ledger{};
};

inline RhythmBeans& GlobalBeans() {
    static RhythmBeans beans{};
    return beans;
}

}  // namespace rhythm::app
