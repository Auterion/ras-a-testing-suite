#pragma once
#include <memory>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/mission/mission.h>
#include <mavsdk/plugins/ftp/ftp.h>
#include <yaml-cpp/yaml.h>
#include "gtest/gtest.h"
#include <chrono>
#include <future>
#include "passthrough_tester.hpp"

namespace RASATestingSuite {

template<typename A, typename B>
A _packUnpack(B b) {
    union U {
        A a;
        B b;
    };
    U u;
    u.b = b;
    return u.a;
}

template<typename T>
T floatUnpack(float f) {
    return _packUnpack<T, float>(f);
}

template<typename T>
float floatPack(T o) {
    return _packUnpack<float, T>(o);
}

class Environment : public ::testing::Environment {
private:
    inline static Environment* _instance;

    const std::string _connection_url;
    YAML::Node _config;
    std::shared_ptr<mavsdk::Mavsdk> _mavsdk;
    std::shared_ptr<mavsdk::System> _system;
    std::shared_ptr<mavsdk::MavlinkPassthrough> _mavlinkPassthrough;
    std::shared_ptr<mavsdk::Mission> _mission;
    std::shared_ptr<mavsdk::Ftp> _ftp;
    std::shared_ptr<PassthroughTester> _tester;

    mavsdk::System::AutopilotVersion _autopilotVersionData;
    TestTargetAddress _test_target;

    static std::shared_ptr<mavsdk::System> getSystem(mavsdk::Mavsdk& mavsdk)
    {
        std::cout << "Waiting to discover system...\n";
        auto prom = std::promise<std::shared_ptr<mavsdk::System>>{};
        auto fut = prom.get_future();

        // We wait for new systems to be discovered, once we find one that has an
        // autopilot, we decide to use it.
        mavsdk.subscribe_on_new_system([&mavsdk, &prom]() {
            auto system = mavsdk.systems().back();

            if (system->has_autopilot()) {
                std::cout << "Discovered autopilot\n";

                // Unsubscribe again as we only want to find one system.
                mavsdk.subscribe_on_new_system(nullptr);
                prom.set_value(system);
            }
        });

        // We usually receive heartbeats at 1Hz, therefore we should find a
        // system after around 3 seconds max, surely.
        if (fut.wait_for(std::chrono::seconds(3)) == std::future_status::timeout) {
            std::cerr << "No autopilot found.\n";
            return {};
        }

        // Get discovered system now.
        return fut.get();
    }

    Environment(const std::string &connection_url, const std::string &yaml_path) : 
    _connection_url(connection_url), _config(YAML::LoadFile(yaml_path)) {
        _test_target = {
            _config["Global"]["system_id"].as<int>(), 
            _config["Global"]["component_id"].as<int>()
        };
    }

public:
    static bool isCreated() {
        return _instance != nullptr;
    }

    static Environment* getInstance() {
        return _instance;
    }

    static void create(const std::string &connection_url, const std::string &yaml_path) {
        if (!isCreated()) {
            _instance = new Environment(connection_url, yaml_path);
        }
    }

    void SetUp() override {
        _mavsdk = std::make_shared<mavsdk::Mavsdk>();
        auto configuration = mavsdk::Mavsdk::Configuration(mavsdk::Mavsdk::Configuration::UsageType::GroundStation);
        configuration.set_system_id(255);
        _mavsdk->set_configuration(configuration);

        mavsdk::ConnectionResult connection_result = _mavsdk->add_any_connection(_connection_url);

        if (connection_result != mavsdk::ConnectionResult::Success) {
            std::cerr << "Connection failed: " << connection_result << '\n';
            throw std::runtime_error("Connection failed");
        }
        _system = getSystem(*_mavsdk);
        if (!_system) {
            throw std::runtime_error("No system found");
        }
        _autopilotVersionData = _system->get_autopilot_version_data();
        _mavlinkPassthrough = std::make_shared<mavsdk::MavlinkPassthrough>(_system);
        _mission = std::make_shared<mavsdk::Mission>(_system);
        _ftp = std::make_shared<mavsdk::Ftp>(_system);
        _tester = std::make_shared<PassthroughTester>(_mavlinkPassthrough);
    }

    std::shared_ptr<mavsdk::System> getSystem() const {
        return _system;
    }

    std::shared_ptr<mavsdk::MavlinkPassthrough> getPassthroughPlugin() const {
        return _mavlinkPassthrough;
    }

    std::shared_ptr<mavsdk::Mission> getMissionPlugin() const {
        return _mission;
    }

    std::shared_ptr<mavsdk::Ftp> getFtpPlugin() const {
        return _ftp;
    }

    std::shared_ptr<PassthroughTester> getPassthroughTester() const {
        return _tester;
    }

    const YAML::Node getConfig(const std::vector<std::string> &path) {
        YAML::Node node = _config;
        for (const auto& key : path) {
            if (!node) {
                printf("KEY NOT IN %s\n", key.c_str());
                return node;
            }
            node.reset(node[key]);
        }
        return node;
    }

    const mavsdk::System::AutopilotVersion& getAutopilotVersion() const {
        return _autopilotVersionData;
    }

    const TestTargetAddress& getTargetAddress() const {
        return _test_target;
    }

    void TearDown() override {
        _tester = nullptr;
        _ftp = nullptr;
        _mission = nullptr;
        _mavlinkPassthrough = nullptr;
        _system = nullptr;
        _mavsdk = nullptr;
    }

    ~Environment() override {
    }
};

};
