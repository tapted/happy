#pragma once

#include <atomic>
#include <mqtt_client.h>

#include "espbase/esp_result.hpp"
#include "happy/device.hpp"

typedef const char* esp_event_base_t;

namespace HAPPY::Transports {

class MqttDevice : public Device {
 public:
  // Inherit the Device constructor
  using Device::Device;
  virtual ~MqttDevice();

  // Start the MQTT client with the given configuration and publish initial states. E.g.:
  // ```cpp
  // void Network::network_ready(const esp_netif_ip_info_t& /*ip_info*/) {
  //   esp_mqtt_client_config_t mqtt_cfg = {};
  //   mqtt_cfg.broker.address.uri = "mqtt://10.1.0.201";
  //   mqtt_cfg.credentials.username = "dongle1e80";
  //   mqtt_cfg.credentials.authentication.password = "mysecretpassword";
  //   dongley_device.begin(mqtt_cfg);
  // }
  // ```
  EspResult<void> begin(const esp_mqtt_client_config_t& mqtt_cfg);

  int poke() override { return pump_queue(false); }

  esp_mqtt_client_handle_t get_client() const { return client_; }

  // Returns true ONLY if the connection has finished and all ACKs are received.
  bool is_idle() const { return pending_acks_.load(std::memory_order_acquire) <= 0; }

 private:
  esp_mqtt_client_handle_t client_ = nullptr;

  // Start at 1 so the device isn't considered idle while it is still trying to connect.
  mutable std::atomic<int> pending_acks_{1};

  static void static_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id,
                                   void* event_data);
  int pump_queue(bool bypass_idle_check);
  void handle_event(int32_t event_id, esp_mqtt_event_handle_t event);

  void on_connected();
  int mqtt_publish(const char* topic, const char* payload, int qos = 1, int retain = 1) const;
  int mqtt_enqueue(const char* topic, const char* payload, int qos = 1, int retain = 1,
                   bool store = false) const;
};

}  // namespace HAPPY::Transports