#include "happy/entities/sensor.hpp"

#include <gtest/gtest.h>

#include "cJSON.h"
#include "happy_test_harness.hpp"

namespace HAPPY::Test {

class SensorEntityTest : public HappyIntegrationTestHarness {};

TEST_F(SensorEntityTest, DiscoveryAndInitialStatePublication) {
  std::string current_val = "23.5";
  HAPPY::Entities::Sensor sensor(device_, "temperature", "Temperature",
                                 {
                                     .device_class = "temperature",
                                     .unit_of_measurement = "°C",
                                     .icon = "mdi:thermometer",
                                     .entity_category = "diagnostic",
                                     .get_value = [&]() { return current_val; },
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

#if 0
  // Parse discovery JSON payload
  cJSON* root = cJSON_Parse(discovery_call->payload.c_str());
  ASSERT_NE(root, nullptr) << "Failed to parse discovery JSON payload";

  cJSON* name_item = cJSON_GetObjectItem(root, "name");
  ASSERT_NE(name_item, nullptr);
  EXPECT_STREQ(name_item->valuestring, "Temperature");

  cJSON* state_topic_item = cJSON_GetObjectItem(root, "state_topic");
  ASSERT_NE(state_topic_item, nullptr);
  EXPECT_STREQ(state_topic_item->valuestring, "test_device/temperature/state");

  cJSON* dev_class_item = cJSON_GetObjectItem(root, "device_class");
  ASSERT_NE(dev_class_item, nullptr);
  EXPECT_STREQ(dev_class_item->valuestring, "temperature");

  cJSON* unit_item = cJSON_GetObjectItem(root, "unit_of_measurement");
  ASSERT_NE(unit_item, nullptr);
  EXPECT_STREQ(unit_item->valuestring, "°C");

  cJSON* dev_obj = cJSON_GetObjectItem(root, "device");
  ASSERT_NE(dev_obj, nullptr);
  cJSON* dev_name = cJSON_GetObjectItem(dev_obj, "name");
  ASSERT_NE(dev_name, nullptr);
  EXPECT_STREQ(dev_name->valuestring, "Test Device");

  cJSON_Delete(root);
#else
  EXPECT_EQ(
      discovery_call->payload,
      R"({"name":"Temperature","state_topic":"test_device/temperature/state","device_class":"temperature","unit_of_measurement":"°C","device":{"identifiers":"test_device","name":"Test Device","manufacturer":"Test Manufacturer","model":"Test Model","sw_version":"1.0.0"}})");
#endif
  // 2. Verify Initial State Topic and Payload
  std::string expected_state_topic = "test_device/temperature/state";
  const EnqueueCall* state_call = find_enqueue_call(expected_state_topic);
  ASSERT_NE(state_call, nullptr) << "State message not enqueued for topic " << expected_state_topic;

  EXPECT_EQ(state_call->payload, "23.5");
}

TEST_F(SensorEntityTest, StateUpdateOnRequestPublish) {
  std::string current_val = "10";
  HAPPY::Entities::Sensor sensor(device_, "uptime", "Uptime",
                                 {
                                     .device_class = "duration",
                                     .unit_of_measurement = "s",
                                     .get_value = [&]() { return current_val; },
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
