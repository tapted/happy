#pragma once

#include <atomic>

#include "espbase/esp_result.hpp"
#include "happy/device.hpp"

typedef const char* esp_event_base_t;
typedef struct esp_mqtt_client_config_t esp_mqtt_client_config_t;
typedef struct esp_mqtt_client* esp_mqtt_client_handle_t;
typedef struct esp_mqtt_event_t esp_mqtt_event_t;
typedef esp_mqtt_event_t* esp_mqtt_event_handle_t;

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

  void poke() override { pump_queue(); }

  esp_mqtt_client_handle_t get_client() const { return client_; }

  bool is_idle() const;

 private:
  esp_mqtt_client_handle_t client_ = nullptr;
  std::atomic<bool> is_connected_{false};
  std::atomic<bool> initial_setup_complete_{false};  // Protects the boot phase
  std::atomic<int> pending_acks_{0};                 // Only tracks active in-flight QoS > 0 packets

  static void static_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id,
                                   void* event_data);
  void pump_queue();
  void handle_event(int32_t event_id, esp_mqtt_event_handle_t event);

  void on_connected();
  int mqtt_publish(const char* topic, const char* payload, int qos = 1, int retain = 1);
  int mqtt_enqueue(const char* topic, const char* payload, int qos = 1, int retain = 1,
                   bool store = false);
};

}  // namespace HAPPY::Transports