#include <gtest/gtest.h>
#include "../environment.hpp"
#include "../passthrough_messages.hpp"

using namespace RASATestingSuite;

class Params : public ::testing::Test {
protected:
    const std::shared_ptr<PassthroughTester> link;
    const TestTargetAddress target;

    Params() : 
    link(Environment::getInstance()->getPassthroughTester()),
    target(Environment::getInstance()->getTargetAddress()) {
        link->flushAll();
    }
};

std::string paramIdString(const char* param_id) {
    return std::string(param_id, strnlen(param_id, 16));
}

TEST_F(Params, ParamReadWriteInteger) {
    auto conf = Environment::getInstance()->getConfig({"Param", "ParamReadWriteInteger"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    auto param_id = conf["param_id"].as<std::string>();
    auto default_value = conf["default_value"].as<int>();
    auto change_value = conf["change_value"].as<int>();

    // Read current value
    link->send<PARAM_REQUEST_READ>(target, param_id.c_str(), -1);
    auto r1 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r1.param_id), param_id) << "Returned param ID does not match requested param ID";
    EXPECT_EQ(floatUnpack<int32_t>(r1.param_value), default_value) << "Returned value for param " << param_id << " does not have configured default value";
    EXPECT_EQ(r1.param_type, MAV_PARAM_TYPE_INT32) << "Returned param type is wrong";

    // Write new value
    link->send<PARAM_SET>(target, param_id.c_str(), floatPack(change_value), MAV_PARAM_TYPE_INT32);
    auto r2 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r2.param_id), param_id) << "Returned param ID does not match requested param ID";

    // Re-read new value
    link->send<PARAM_REQUEST_READ>(target, param_id.c_str(), -1);
    auto r3 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r3.param_id), param_id) << "Returned param ID does not match requested param ID";
    EXPECT_EQ(floatUnpack<int32_t>(r3.param_value), change_value) << "Returned value for param " << param_id << " is not changed value";
    EXPECT_EQ(r3.param_type, MAV_PARAM_TYPE_INT32) << "Returned param type is wrong";

    // Restore default value
    link->send<PARAM_SET>(target, param_id.c_str(), floatPack(default_value), MAV_PARAM_TYPE_INT32);
    auto r4 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r4.param_id), param_id) << "Returned param ID does not match requested param ID";
}

TEST_F(Params, ParamReadWriteFloat) {
    auto conf = Environment::getInstance()->getConfig({"Param", "ParamReadWriteFloat"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    auto param_id = conf["param_id"].as<std::string>();
    auto default_value = conf["default_value"].as<float>();
    auto change_value = conf["change_value"].as<float>();

    // Read current value
    link->send<PARAM_REQUEST_READ>(target, param_id.c_str(), -1);
    auto r1 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r1.param_id), param_id) << "Returned param ID does not match requested param ID";
    EXPECT_EQ(floatUnpack<float>(r1.param_value), default_value) << "Returned value for param " << param_id << " does not have configured default value";
    EXPECT_EQ(r1.param_type, MAV_PARAM_TYPE_REAL32) << "Returned param type is wrong";

    // Write new value
    link->send<PARAM_SET>(target, param_id.c_str(), floatPack(change_value), MAV_PARAM_TYPE_REAL32);
    auto r2 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r2.param_id), param_id) << "Returned param ID does not match requested param ID";

    // Re-read new value
    link->send<PARAM_REQUEST_READ>(target, param_id.c_str(), -1);
    auto r3 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r3.param_id), param_id) << "Returned param ID does not match requested param ID";
    EXPECT_EQ(floatUnpack<float>(r3.param_value), change_value) << "Returned value for param " << param_id << " is not changed value";
    EXPECT_EQ(r3.param_type, MAV_PARAM_TYPE_REAL32) << "Returned param type is wrong";

    // Restore default value
    link->send<PARAM_SET>(target, param_id.c_str(), floatPack(default_value), MAV_PARAM_TYPE_REAL32);
    auto r4 = link->receive<PARAM_VALUE>(target);
    EXPECT_EQ(paramIdString(r4.param_id), param_id) << "Returned param ID does not match requested param ID";
}

TEST_F(Params, ParamListAll) {
    auto conf = Environment::getInstance()->getConfig({"Param", "ParamListAll"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    link->send<PARAM_REQUEST_LIST>(target);
    int count = 0;

    std::set<std::string> received_param_ids;
    do {
        auto r = link->receive<PARAM_VALUE>(target);
        count = r.param_count;
        // The _HASH_CHECK param does not count towards the total count
        if (paramIdString(r.param_id) != "_HASH_CHECK") {
            received_param_ids.insert(paramIdString(r.param_id));
        }
    } while(static_cast<int>(received_param_ids.size()) < count);

    EXPECT_EQ(received_param_ids.size(), count) << "Did not receive all params";
    // next receive should time out
    try {
        while (true) {
            // we expect this to time out.
            // if it does not (we receive another param), it is only okay if we already have
            // that param
            auto extra = link->receive<PARAM_VALUE>(target);
            if (received_param_ids.find(paramIdString(extra.param_id)) == received_param_ids.end()) {
                FAIL() << "Received more params. Extra param " << paramIdString(extra.param_id);
            }
        }
    } catch(TimeoutError&) {}
}

