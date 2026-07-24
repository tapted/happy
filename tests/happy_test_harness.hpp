#pragma once

#include <gtest/gtest.h>

#include "happy/transports/mqtt_device.hpp"
#include "mocks/mock_mqtt.hpp"

namespace HAPPY::Test {

class HappyIntegrationTestHarness : public ::testing::Test {
 protected:
  HAPPY::Device::Config device_config_{
      .identifiers = "test_device",
      .name = "Test Device",
      .manufacturer = "Test Manufacturer",
      .model = "Test Model",
      .sw_version = "1.0.0",
  };

  HAPPY::Transports::MqttDevice device_{device_config_};

  void SetUp() override { MockMqtt::instance().reset(); }

  void TearDown() override { MockMqtt::instance().reset(); }

  void start_device_and_connect() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    EXPECT_TRUE(device_.begin(mqtt_cfg));
    MockMqtt::instance().simulate_connect();
  }

  void connect_and_pump_until_idle(int max_iterations = 100) {
    start_device_and_connect();
    MockMqtt::instance().pump_until_idle_or_max(device_, max_iterations);
  }

  void pump_until_idle(int max_iterations = 100) {
    MockMqtt::instance().pump_until_idle_or_max(device_, max_iterations);
  }

  const std::vector<EnqueueCall>& enqueue_calls() const {
    return MockMqtt::instance().enqueue_calls();
  }

  const std::vector<SubscribeCall>& subscribe_calls() const {
    return MockMqtt::instance().subscribe_calls();
  }

  const std::vector<PublishCall>& publish_calls() const {
    return MockMqtt::instance().publish_calls();
  }

  const EnqueueCall* find_enqueue_call(const std::string& topic) const {
    for (const auto& call : enqueue_calls()) {
      if (call.topic == topic) return &call;
    }
    return nullptr;
  }

  const SubscribeCall* find_subscribe_call(const std::string& topic) const {
    for (const auto& call : subscribe_calls()) {
      if (call.topic == topic) return &call;
    }
    return nullptr;
  }
};

}  // namespace HAPPY::Test
