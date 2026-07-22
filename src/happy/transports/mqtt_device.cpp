#include "happy/transports/mqtt_device.hpp"

#include <esp_log.h>

#include "happy/entity.hpp"

namespace HAPPY::Transports {

static constexpr char TAG[] = "MqttDevice";

MqttDevice::~MqttDevice() {
  if (client_) {
    esp_mqtt_client_stop(client_);
    esp_mqtt_client_destroy(client_);
  }
}

EspResult<void> MqttDevice::begin(const esp_mqtt_client_config_t& mqtt_cfg) {
  // 1. Run the two-phase initialization to allocate topics safely
  Device::begin();

  client_ = esp_mqtt_client_init(&mqtt_cfg);
  if (!client_) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return ESP_ERR_NO_MEM;
  }

  if (EspError err = esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY,
                                                    &MqttDevice::static_event_handler, this)) {
    return err.log(TAG, "Failed to register MQTT event handler");
  }

  if (EspError err = esp_mqtt_client_start(client_)) {
    return err.log(TAG, "Failed to start MQTT client");
  }
  return ESP_OK;
}

// FreeRTOS requires a static C-style function signature. We use the handler_args
// to cast the pointer back into our specific C++ instance.
void MqttDevice::static_event_handler(void* handler_args, esp_event_base_t /*base*/,
                                      int32_t event_id, void* event_data) {
  MqttDevice* instance = static_cast<MqttDevice*>(handler_args);
  esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

  instance->handle_event(event_id, event);
}

int MqttDevice::pump_queue(bool is_ack_resolution) {
  // Define your maximum packets in flight (sliding window size)
  constexpr int MAX_IN_FLIGHT = 1;
  int num_published = 0;

  for (Entity& entity : entities_) {
    // 1. Evaluate the real-time window capacity
    int current_in_flight = pending_acks_.load(std::memory_order_acquire);

    // If we are currently handling an ACK, one of those in-flight locks is about to be released
    // immediately after this function returns. We account for it to maximize our window.
    if (is_ack_resolution) current_in_flight -= 1;

    // If the window is full (or permanently locked by a disconnect), stop pumping.
    if (current_in_flight >= MAX_IN_FLIGHT) return num_published;

    // 2. Dispatch the next highest-priority action
    if (entity.get_pending_flags() & Entity::FLAG_DISCOVERY) {
      entity.clear_flag(Entity::FLAG_DISCOVERY);
      // mqtt_publish automatically increments pending_acks_ under the hood
      mqtt_publish(entity.get_discovery_topic().c_str(), entity.get_discovery_payload().c_str(), 1,
                   1);

    } else if (entity.get_pending_flags() & Entity::FLAG_SUBSCRIBE) {
      entity.clear_flag(Entity::FLAG_SUBSCRIBE);
      int msg_id = esp_mqtt_client_subscribe_single(client_, entity.get_command_topic().c_str(), 1);
      if (msg_id > 0) {
        pending_acks_.fetch_add(1, std::memory_order_relaxed);
      } else if (msg_id == -2) {
        ESP_LOGW(TAG, "Outbox full. Failed to subscribe to topic: %s",
                 entity.get_command_topic().c_str());
      } else if (msg_id == -1) {
        ESP_LOGW(TAG, "Failed to subscribe to topic: %s", entity.get_command_topic().c_str());
      }

    } else if (entity.get_pending_flags() & Entity::FLAG_STATE) {
      entity.clear_flag(Entity::FLAG_STATE);
      // Pushing state data at QoS 0 bypasses the outbox queue entirely
      mqtt_publish(entity.get_state_topic().c_str(), entity.get_state_payload().c_str(), 0, 0);
    }

    ++num_published;
  }
  return num_published;
}

// --- 3. The Object-Oriented Event Router ---
void MqttDevice::handle_event(int32_t event_id, esp_mqtt_event_handle_t event) {
  switch (event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
      break;  // We're running!

    case MQTT_EVENT_CONNECTED:
      ESP_LOGI("MqttDevice", "Connected to Broker. Queueing Discovery...");
      on_connected();
      // Remove the initialization lock
      pending_acks_.fetch_sub(1, std::memory_order_release);
      break;

    case MQTT_EVENT_DATA: {
      // Wrap the raw C buffers in zero-allocation C++ views
      std::string_view topic(event->topic, event->topic_len);
      std::string_view payload(event->data, event->data_len);
      ESP_LOGI(TAG, "Received MQTT message on topic: %.*s, payload: %.*s",
               static_cast<int>(topic.size()), topic.data(), static_cast<int>(payload.size()),
               payload.data());

      // Pass directly to the base registry for routing
      this->dispatch_command(topic, payload);
      break;
    }

    case MQTT_EVENT_SUBSCRIBED:
    case MQTT_EVENT_PUBLISHED:
      // Immediately check if another entity is waiting to send data
      pump_queue(true);  // bypass idle check to avoid idle detection
      // The broker acknowledged our last packet!
      pending_acks_.fetch_sub(1, std::memory_order_release);
      break;

    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGW("MqttDevice", "Disconnected from Broker.");
      // Lock the pump so entities can flag themselves, but pump_queue() exits early
      if (pending_acks_.load(std::memory_order_acquire) <= 0) {
        pending_acks_.fetch_add(1, std::memory_order_release);
      }
      break;

    default:
      ESP_LOGW(TAG, "Unhandled MQTT Event: %d", event_id);
      break;
  }
}

// --- 4. The Auto-Discovery Engine ---
void MqttDevice::on_connected() {
  // Flag everything for setup
  for (Entity& entity : entities_) {
    entity.request_discovery();
    entity.request_publish();  // Ensure the latest state is sent on boot
  }
  // Kick off the first packet
  pump_queue(true);
}

int MqttDevice::mqtt_publish(const char* topic, const char* payload, int qos, int retain) const {
  int msg_id = esp_mqtt_client_publish(client_, topic, payload, 0, qos, retain);
  if (msg_id > 0) {
    pending_acks_.fetch_add(1, std::memory_order_relaxed);
  } else if (msg_id == -2) {
    ESP_LOGW(TAG, "Outbox full. Failed to publish message to topic: %s", topic);
  } else if (msg_id == -1) {
    ESP_LOGW(TAG, "Failed to publish message to topic: %s", topic);
  }

  return msg_id;
}

// Note: unused.
int MqttDevice::mqtt_enqueue(const char* topic, const char* payload, int qos, int retain,
                             bool store) const {
  int msg_id = esp_mqtt_client_enqueue(client_, topic, payload, 0, qos, retain, store);
  if (msg_id > 0) {
    pending_acks_.fetch_add(1, std::memory_order_relaxed);
  } else if (msg_id == -2) {
    ESP_LOGW(TAG, "Outbox full. Failed to enqueue message to topic: %s", topic);
  } else if (msg_id == -1) {
    ESP_LOGW(TAG, "Failed to enqueue message to topic: %s", topic);
  }
  return msg_id;
}

}  // namespace HAPPY::Transports