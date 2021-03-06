#include <gtest/gtest.h>
#include "../environment.hpp"
#include "../passthrough_messages.hpp"

using namespace RASATestingSuite;

struct int_coord_t {
    int32_t latitude;
    int32_t longitude;
    float altitude;
};


class Mission : public ::testing::Test {
protected:
    const std::shared_ptr<PassthroughTester> link;
    const YAML::Node config;
    const TestTargetAddress target;

    Mission() :
          link(Environment::getInstance()->getPassthroughTester()),
          config(Environment::getInstance()->getConfig({"Mission"})),
          target(Environment::getInstance()->getTargetAddress()) {
        link->flushAll();
    }


    bool hasCapability(uint64_t capability) const {
        return (Environment::getInstance()->getAutopilotVersion().capabilities & capability) == capability;
    }

    // Dummy coordinate generator
    int_coord_t missionCoordGen(int seq) {
        float altitude = 10.F + (float)seq;
        double latitude = config["home_lat"].as<double>() + (double)seq * 1e-5;
        double longitude = config["home_lon"].as<double>();
        return {
            static_cast<int32_t>(latitude * 1e7),
            static_cast<int32_t>(longitude * 1e7),
            altitude
        };
    }

    int_coord_t fenceCoordGen(int seq) {
        if (seq > 3 || seq <0) {
            throw std::runtime_error("sequence out of bounds");
        }
        static constexpr int lat_weights[] = {-1, -1, 1, 1};
        static constexpr int lon_weights[] = {-1, 1, 1, -1};

        double latitude = config["home_lat"].as<double>() + lat_weights[seq] * 1e-5;
        double longitude = config["home_lon"].as<double>() + lon_weights[seq] * 1e-5;
        return {
            static_cast<int32_t>(latitude * 1e7),
            static_cast<int32_t>(longitude * 1e7),
            0.0f
        };
    }

    void uploadMission(int N_ITEMS=10) {
        EXPECT_TRUE(hasCapability(MAV_PROTOCOL_CAPABILITY_COMMAND_INT)) << "MISSION_INT capability not reported";
        link->send<MISSION_COUNT>(target, N_ITEMS, MAV_MISSION_TYPE_MISSION);
        auto req = link->receive<MISSION_REQUEST_INT>(target);
        EXPECT_EQ(req.seq, 0);

        for (int i=0; i<N_ITEMS; i++) {
            auto c = missionCoordGen(i);
            link->send<MISSION_ITEM_INT>(target, i, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_WAYPOINT, 0, 1,
                                         0.f, 1.f, 0.f, NAN,
                                         c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);
            if (i<N_ITEMS -1) {
                req = link->receive<MISSION_REQUEST_INT>(target);
                EXPECT_EQ(req.seq, i+1);
            } else {
                auto ack = link->receive<MISSION_ACK>(1,1);
                EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Mission not accepted" << std::endl;
            }
        }
    }

    void downloadMission(int N_ITEMS=10) {
        link->send<MISSION_REQUEST_LIST>(target, MAV_MISSION_TYPE_MISSION);
        auto cnt = link->receive<MISSION_COUNT>(target);

        EXPECT_EQ(cnt.count, N_ITEMS) << "Received wrong mission count" << std::endl;
        EXPECT_EQ(cnt.mission_type, MAV_MISSION_TYPE_MISSION) << "Received count for wrong mission type" << std::endl;
        for (int i=0; i<N_ITEMS; i++) {
            link->send<MISSION_REQUEST_INT>(target, i, MAV_MISSION_TYPE_MISSION);
            auto item = link->receive<MISSION_ITEM_INT>(target);
            checkMissionItem(item, i);
        }
        link->send<MISSION_ACK>(target, MAV_MISSION_ACCEPTED, MAV_MISSION_TYPE_MISSION);
    }


    void checkMissionItem(const mavlink_mission_item_int_t &item, int seq) {
        EXPECT_EQ(item.command, MAV_CMD_NAV_WAYPOINT);
        EXPECT_EQ(item.x, missionCoordGen(seq).latitude);
        EXPECT_EQ(item.y, missionCoordGen(seq).longitude);
        EXPECT_EQ(item.z, missionCoordGen(seq).altitude);
        EXPECT_EQ(item.mission_type, MAV_MISSION_TYPE_MISSION);
        EXPECT_EQ(item.seq, seq);
    }

    void clearAll() {
        link->send<MISSION_CLEAR_ALL>(target, MAV_MISSION_TYPE_ALL);
        auto ack = link->receive<MISSION_ACK>(target);
        EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED);
    }
};

TEST_F(Mission, Upload) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "Upload"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    clearAll();
    uploadMission();
    clearAll();
}

TEST_F(Mission, UploadAndDownload) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadAndDownload"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    const int N_ITEMS = 10;

    uploadMission(N_ITEMS);
    downloadMission(N_ITEMS);
    clearAll();
}

TEST_F(Mission, SetCurrentItem) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "SetCurrentItem"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    uploadMission();
    link->send<MISSION_SET_CURRENT>(target, 2);
    // drop all queued MISSION_CURRENT messages
    link->flush<MISSION_CURRENT>(target);
    // MISSION_CURRENT is potentially sent at high rate. It could be some
    // messages until we observe the change, so we observe the next 10 messages
    const int OBSERVE_N = 10;

    EXPECT_TRUE(link->expectCondition<MISSION_CURRENT>(target, OBSERVE_N, 500, [](auto curr){
       return curr.seq == 2;
    }));
    clearAll();
}


TEST_F(Mission, UploadPolygonFence) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadPolygonFence"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    EXPECT_TRUE(hasCapability(MAV_PROTOCOL_CAPABILITY_MISSION_FENCE)) << "MISSION_FENCE capability not reported";

    link->send<MISSION_COUNT>(target, 4, MAV_MISSION_TYPE_FENCE);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);

    // send inclusion fence
    for (int i=0; i<4; i++) {
        auto c = fenceCoordGen(i);
        link->send<MISSION_ITEM_INT>(target, i, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_FENCE_POLYGON_VERTEX_INCLUSION, 0, 0,
                                     4.f, 1.f, NAN, NAN,
                                     c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_FENCE);
        if (i<3) {
            req = link->receive<MISSION_REQUEST_INT>(target);
            EXPECT_EQ(req.seq, i+1);
        } else {
            auto ack = link->receive<MISSION_ACK>(target);
            EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Mission not accepted" << std::endl;
        }
    }

    link->send<MISSION_COUNT>(target, 4, MAV_MISSION_TYPE_FENCE);
    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    // send exclusion fence
    for (int i=0; i<4; i++) {
        auto c = fenceCoordGen(i);
        link->send<MISSION_ITEM_INT>(target, i, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_FENCE_POLYGON_VERTEX_EXCLUSION, 0, 0,
                                     4.f, 2.f, NAN, NAN,
                                     c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_FENCE);
        if (i<3) {
            req = link->receive<MISSION_REQUEST_INT>(target);
            EXPECT_EQ(req.seq, i+1);
        } else {
            auto ack = link->receive<MISSION_ACK>(target);
            EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Fence not accepted" << std::endl;
        }
    }

    clearAll();
}

TEST_F(Mission, UploadCircularFence) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadCircularFence"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    EXPECT_TRUE(hasCapability(MAV_PROTOCOL_CAPABILITY_MISSION_FENCE)) << "MISSION_FENCE capability not reported";

    double latitude = config["home_lat"].as<double>();
    double longitude = config["home_lon"].as<double>();

    link->send<MISSION_COUNT>(target, 2, MAV_MISSION_TYPE_FENCE);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    link->send<MISSION_ITEM_INT>(target, 0, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_FENCE_CIRCLE_INCLUSION, 0, 0,
                                 100.f, 1.f, NAN, NAN,
                                 latitude, longitude, 0, MAV_MISSION_TYPE_FENCE);
    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 1);
    link->send<MISSION_ITEM_INT>(target, 1, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_FENCE_CIRCLE_EXCLUSION, 0, 0,
                                 20.f, 1.f, NAN, NAN,
                                 latitude, longitude, 0, MAV_MISSION_TYPE_FENCE);
    auto ack = link->receive<MISSION_ACK>(target);
    EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Fence not accepted" << std::endl;
    clearAll();
}

TEST_F(Mission, UploadRallyPoints) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadRallyPoints"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }
    EXPECT_TRUE(hasCapability(MAV_PROTOCOL_CAPABILITY_MISSION_RALLY)) << "MISSION_RALLY capability not reported";

    link->send<MISSION_COUNT>(target, 1, MAV_MISSION_TYPE_RALLY);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    auto c = fenceCoordGen(0);
    link->send<MISSION_ITEM_INT>(target, 0, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_RALLY_POINT, 0, 1,
                                 NAN, NAN, NAN, NAN, c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_RALLY);
    auto ack = link->receive<MISSION_ACK>(target);
    EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Rally point not accepted" << std::endl;
    clearAll();
}

TEST_F(Mission, UploadTakeoffLoiterMission) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadTakeoffLoiterMission"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }

    link->send<MISSION_COUNT>(target, 2, MAV_MISSION_TYPE_MISSION);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    auto c = missionCoordGen(0);

    link->send<MISSION_ITEM_INT>(target, 0, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_TAKEOFF, 0, 1,
                                     0.f, 1.f, 0.f, NAN,
                                     c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);
    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 1);

    c = missionCoordGen(1);
    link->send<MISSION_ITEM_INT>(target, 1, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_LOITER_UNLIM, 0, 1,
                                 0.f, 1.f, 0.f, NAN,
                                 c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);

    auto ack = link->receive<MISSION_ACK>(target);
    EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Mission not accepted" << std::endl;
}

TEST_F(Mission, UploadTakeoffLandMission) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadTakeoffLandMission"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }

    link->send<MISSION_COUNT>(target, 2, MAV_MISSION_TYPE_MISSION);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    auto c = missionCoordGen(0);

    link->send<MISSION_ITEM_INT>(target, 0, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_TAKEOFF, 0, 1,
                                 0.f, 1.f, 0.f, NAN,
                                 c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);
    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 1);

    c = missionCoordGen(1);
    link->send<MISSION_ITEM_INT>(target, 1, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_LAND, 0, 1,
                                 0.f, 0.f, 0.f, NAN,
                                 c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);

    auto ack = link->receive<MISSION_ACK>(target);
    EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Mission not accepted" << std::endl;
}

TEST_F(Mission, UploadTakeoffChangeSpeedReturn) {
    auto conf = Environment::getInstance()->getConfig({"Mission", "UploadTakeoffReturn"});
    if (!conf || conf["skip"].as<bool>(false)) {
        GTEST_SKIP();
    }

    link->send<MISSION_COUNT>(target, 3, MAV_MISSION_TYPE_MISSION);
    auto req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 0);
    auto c = missionCoordGen(0);

    link->send<MISSION_ITEM_INT>(target, 0, MAV_FRAME_GLOBAL_INT, MAV_CMD_NAV_TAKEOFF, 0, 1,
                                 0.f, 1.f, 0.f, NAN,
                                 c.latitude, c.longitude, c.altitude, MAV_MISSION_TYPE_MISSION);
    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 1);

    link->send<MISSION_ITEM_INT>(target, 1, MAV_FRAME_MISSION, MAV_CMD_DO_CHANGE_SPEED, 0, 1,
                                 1.f, 5.f, -1.f,
                                 NAN, UINT32_MAX, UINT32_MAX, UINT32_MAX, MAV_MISSION_TYPE_MISSION);

    req = link->receive<MISSION_REQUEST_INT>(target);
    EXPECT_EQ(req.seq, 2);

    c = missionCoordGen(1);
    link->send<MISSION_ITEM_INT>(target, 2, MAV_FRAME_MISSION, MAV_CMD_NAV_RETURN_TO_LAUNCH, 0, 1,
                                 NAN, NAN, NAN, NAN,
                                 UINT32_MAX, UINT32_MAX, UINT32_MAX, MAV_MISSION_TYPE_MISSION);

    auto ack = link->receive<MISSION_ACK>(target);
    EXPECT_EQ(ack.type, MAV_MISSION_ACCEPTED) << "Mission not accepted" << std::endl;
}
