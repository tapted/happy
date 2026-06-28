#pragma once

#include <mqtt_client.h>

#include "happy/device.hpp"

typedef const char* esp_event_base_t;

namespace HAPPY::Transports {

class MqttDevice : public Device {
 public:
  // Inherit the Device constructor
  using Device::Device;

  // --- 1. The Startup Sequence ---
  void begin(const esp_mqtt_client_config_t& mqtt_cfg);
  void publish(const Entity& entity) const override;

  esp_mqtt_client_handle_t get_client() const { return client_; }

 private:
  esp_mqtt_client_handle_t client_ = nullptr;

  static void static_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id,
                                   void* event_data);
  void handle_event(int32_t event_id, esp_mqtt_event_handle_t event);

  void on_connected();
};

}  // namespace HAPPY::Transports