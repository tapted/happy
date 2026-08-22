#include "happy/entities/sensor.hpp"

#include <gtest/gtest.h>

#include <ArduinoJson.h>
#include "espbase/stack_json/buffer.hpp"
#include "espbase/stack_json/pretty_buffer.hpp"
#include "happy_test_harness.hpp"

namespace HAPPY::Test {

class SensorEntityTest : public HappyIntegrationTestHarness {};

static std::string current_val;  // Global variable to simulate sensor value

TEST_F(SensorEntityTest, Payload) {
  current_val = "23.5";
  HAPPY::Entities::Sensor sensor(device_, "temperature", "Temperature",
                                 {
                                     .device_class = "temperature",
                                     .unit_of_measurement = "°C",
                                     .icon = "mdi:thermometer",
                                     .entity_category = "diagnostic",
                                     .get_value = [](auto*) { return current_val; },
                                 });

  // connect_and_pump_until_idle();

  sjson::StackBuffer<1024> buffer;
  sjson::PrettyBuffer pretty(buffer, 2);
  EXPECT_TRUE(sensor.get_discovery_payload(pretty));
  EXPECT_STREQ(
      buffer.c_str(),
      R"({
  "device_class": "temperature",
  "unit_of_measurement": "°C",
  "icon": "mdi:thermometer",
  "entity_category": "diagnostic",
  "name": "Temperature",
  "unique_id": "test_device_temperature",
  "state_topic": "test_device/temperature/state",
  "device": {
    "identifiers": [
      "test_device"
    ],
    "name": "Test Device",
    "manufacturer": "Test Manufacturer",
    "model": "Test Model",
    "sw_version": "1.0.0"
  }
})");
}

TEST_F(SensorEntityTest, DiscoveryAndInitialStatePublication) {
  current_val = "23.5";
  HAPPY::Entities::Sensor sensor(device_, "temperature", "Temperature",
                                 {
                                     .device_class = "temperature",
                                     .unit_of_measurement = "°C",
                                     .icon = "mdi:thermometer",
                                     .entity_category = "diagnostic",
                                     .get_value = [](auto*) { return current_val; },
                                 });

  // Connect and drive the MQTT state machine until all packets are sent and ACKed
  connect_and_pump_until_idle();

  // 1. Verify Discovery Topic and Payload
  std::string expected_discovery_topic = "homeassistant/sensor/test_device/temperature/config";
  const EnqueueCall* discovery_call = find_enqueue_call(expected_discovery_topic);
  ASSERT_NE(discovery_call, nullptr)
      << "Discovery message not enqueued for topic " << expected_discovery_topic;

  EXPECT_EQ(discovery_call->qos, 1);
  EXPECT_EQ(discovery_call->retain, 1);

  // Parse discovery JSON payload using ArduinoJson
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, discovery_call->payload);
  ASSERT_FALSE(err) << "Failed to parse discovery JSON payload: " << err.c_str();

  EXPECT_EQ(doc["name"], "Temperature");
  EXPECT_EQ(doc["state_topic"], "test_device/temperature/state");
  EXPECT_EQ(doc["device_class"], "temperature");
  EXPECT_EQ(doc["unit_of_measurement"], "°C");
  EXPECT_EQ(doc["icon"], "mdi:thermometer");
  EXPECT_EQ(doc["entity_category"], "diagnostic");
  EXPECT_EQ(doc["unique_id"], "test_device_temperature");

  JsonObject dev_obj = doc["device"];
  ASSERT_FALSE(dev_obj.isNull());
  EXPECT_EQ(dev_obj["name"], "Test Device");
  EXPECT_EQ(dev_obj["manufacturer"], "Test Manufacturer");
  EXPECT_EQ(dev_obj["model"], "Test Model");
  EXPECT_EQ(dev_obj["sw_version"], "1.0.0");
  EXPECT_EQ(dev_obj["identifiers"][0], "test_device");

  // 2. Verify Initial State Topic and Payload
  std::string expected_state_topic = "test_device/temperature/state";
  const EnqueueCall* state_call = find_enqueue_call(expected_state_topic);
  ASSERT_NE(state_call, nullptr) << "State message not enqueued for topic " << expected_state_topic;

  EXPECT_EQ(state_call->payload, "23.5");
}

TEST_F(SensorEntityTest, StateUpdateOnRequestPublish) {
  current_val = "10";
  HAPPY::Entities::Sensor sensor(device_, "uptime", "Uptime",
                                 {
                                     .device_class = "duration",
                                     .unit_of_measurement = "s",
                                     .get_value = [](auto*) { return current_val; },
                                 });

  connect_and_pump_until_idle();

  size_t initial_call_count = enqueue_calls().size();

  current_val = "20";
  sensor.request_publish();

  pump_until_idle();

  ASSERT_GT(enqueue_calls().size(), initial_call_count);
  const auto& last_call = enqueue_calls().back();
  EXPECT_EQ(last_call.topic, "test_device/uptime/state");
  EXPECT_EQ(last_call.payload, "20");
}

}  // namespace HAPPY::Test
