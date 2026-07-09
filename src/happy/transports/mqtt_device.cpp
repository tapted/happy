#include "happy/transports/mqtt_device.hpp"

#include <esp_log.h>

#include "happy/entity.hpp"

namespace HAPPY::Transports {

EspResult<void> MqttDevice::begin(const esp_mqtt_client_config_t& mqtt_cfg) {
  // 1. Run the two-phase initialization to allocate topics safely
  Device::begin();

  client_ = esp_mqtt_client_init(&mqtt_cfg);
  if (!client_) {
    ESP_LOGE("MqttDevice", "Failed to initialize MQTT client");
    return ESP_ERR_NO_MEM;
  }

  if (EspError err = esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY,
                                                    &MqttDevice::static_event_handler, this)) {
    return err.log("MqttDevice", "Failed to register MQTT event handler");
  }

  if (EspError err = esp_mqtt_client_start(client_)) {
    return err.log("MqttDevice", "Failed to start MQTT client");
  }
  return ESP_OK;
}

int MqttDevice::publish(const Entity& entity) const {
  std::string payload = entity.get_state_payload();
  return mqtt_enqueue(entity.get_state_topic().c_str(), payload.c_str());
}

// FreeRTOS requires a static C-style function signature. We use the handler_args
// to cast the pointer back into our specific C++ instance.
void MqttDevice::static_event_handler(void* handler_args, esp_event_base_t /*base*/,
                                      int32_t event_id, void* event_data) {
  MqttDevice* instance = static_cast<MqttDevice*>(handler_args);
  esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

  instance->handle_event(event_id, event);
}

// --- 3. The Object-Oriented Event Router ---
void MqttDevice::handle_event(int32_t event_id, esp_mqtt_event_handle_t event) {
  switch (event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
      break;  // We're running!

    case MQTT_EVENT_CONNECTED:
      ESP_LOGI("MqttDevice", "Connected to Broker. Publishing Discovery...");
      on_connected();
      break;

    case MQTT_EVENT_DATA: {
      // Wrap the raw C buffers in zero-allocation C++ views
      std::string_view topic(event->topic, event->topic_len);
      std::string_view payload(event->data, event->data_len);

      // Pass directly to the base registry for routing
      this->dispatch_command(topic, payload);
      break;
    }

    // Responses to our events.
    case MQTT_EVENT_SUBSCRIBED:
    case MQTT_EVENT_PUBLISHED:
      break;

    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGW("MqttDevice", "Disconnected from Broker.");
      break;

    default:
      ESP_LOGW("MqttDevice", "Unhandled MQTT Event: %d", event_id);
      break;
  }
}

// --- 4. The Auto-Discovery Engine ---
void MqttDevice::on_connected() {
  // Iterate over the IntrusiveList natively
  for (Entity& entity : entities_) {
    // 1. Publish the JSON Discovery Payload (Retained = true)
    std::string payload = entity.get_discovery_payload();
    ESP_LOGD("MqttDevice", "Publishing Discovery for %s: %s", entity.get_discovery_topic().c_str(),
             payload.c_str());
    mqtt_publish(entity.get_discovery_topic().c_str(), payload.c_str());

    // 2. Subscribe to the command topic if the entity has one (e.g., Lights, Switches)
    if (!entity.get_command_topic().empty()) {
      ESP_LOGD("MqttDevice", "Subscribing to command topic: %s",
               entity.get_command_topic().c_str());
      esp_mqtt_client_subscribe_single(client_, entity.get_command_topic().c_str(), 1);
    }
  }

  for (const Entity& entity : entities_) {
    publish(entity);
  }
}

int MqttDevice::mqtt_publish(const char* topic, const char* payload, int qos, int retain) const {
  return esp_mqtt_client_publish(client_, topic, payload, 0, qos, retain);
}

int MqttDevice::mqtt_enqueue(const char* topic, const char* payload, int qos, int retain,
                             bool store) const {
  return esp_mqtt_client_enqueue(client_, topic, payload, 0, qos, retain, store);
}

}  // namespace HAPPY::Transports