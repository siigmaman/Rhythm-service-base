#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "services/action_service.hpp"

namespace {

class CaptureQueue final : public rhythm::infra::IEventQueuePort {
   public:
    void PublishJson(const std::string& routing_key, const std::string& json_payload) override {
        keys.push_back(routing_key);
        bodies.push_back(json_payload);
    }
    std::vector<std::string> keys;
    std::vector<std::string> bodies;
};

class AlwaysReserveIdem final : public rhythm::infra::IIdempotencyPort {
   public:
    bool TryReserve(const std::string&, std::chrono::seconds) override { return true; }
};

class NeverReserveIdem final : public rhythm::infra::IIdempotencyPort {
   public:
    bool TryReserve(const std::string&, std::chrono::seconds) override { return false; }
};

}  // namespace

TEST(ActionService, AcceptsValidPayloadAndPublishes) {
    CaptureQueue q;
    AlwaysReserveIdem idem;
    action_service::ActionService svc(q, idem);
    action_service::ActionPayload p;
    p.user_id = "u";
    p.action_type = "view";
    p.product_id = "1";
    p.idempotency_key = "k1";
    EXPECT_EQ(svc.HandleAction(p), action_service::HandleActionResult::Accepted);
    ASSERT_EQ(q.keys.size(), 1u);
    EXPECT_EQ(q.keys[0], "user.action");
    EXPECT_NE(q.bodies[0].find("\"user_id\":\"u\""), std::string::npos);
    EXPECT_NE(q.bodies[0].find("\"idempotency_key\":\"k1\""), std::string::npos);
}

TEST(ActionService, DuplicateWhenIdempotencyFails) {
    CaptureQueue q;
    NeverReserveIdem idem;
    action_service::ActionService svc(q, idem);
    action_service::ActionPayload p{"u", "view", "1", "k"};
    EXPECT_EQ(svc.HandleAction(p), action_service::HandleActionResult::Duplicate);
    EXPECT_TRUE(q.keys.empty());
}

TEST(ActionService, RejectsBadActionType) {
    CaptureQueue q;
    AlwaysReserveIdem idem;
    action_service::ActionService svc(q, idem);
    action_service::ActionPayload p{"u", "click", "1", "k"};
    EXPECT_THROW(svc.HandleAction(p), std::runtime_error);
}

TEST(ActionService, AcceptsLikeAndSave) {
    CaptureQueue q;
    AlwaysReserveIdem idem;
    action_service::ActionService svc(q, idem);
    action_service::ActionPayload like{"u", "like", "1", "k-like"};
    action_service::ActionPayload save{"u", "save", "2", "k-save"};
    EXPECT_EQ(svc.HandleAction(like), action_service::HandleActionResult::Accepted);
    EXPECT_EQ(svc.HandleAction(save), action_service::HandleActionResult::Accepted);
    ASSERT_EQ(q.bodies.size(), 2u);
    EXPECT_NE(q.bodies[0].find("\"action_type\":\"like\""), std::string::npos);
    EXPECT_NE(q.bodies[1].find("\"action_type\":\"save\""), std::string::npos);
}

TEST(ActionService, RejectsEmptyUserId) {
    CaptureQueue q;
    AlwaysReserveIdem idem;
    action_service::ActionService svc(q, idem);
    action_service::ActionPayload p{"", "view", "1", "k"};
    EXPECT_THROW(svc.HandleAction(p), std::runtime_error);
}
