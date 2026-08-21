#include "happy/transports/mqtt_device.hpp"

#include <esp_log.h>
#include <mqtt_client.h>

#include "espbase/stack_json/buffer.hpp"
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

bool MqttDevice::is_idle() const {
  // 1. Not idle if we are still booting and haven't queued our first discovery
  if (!initial_setup_complete_.load(std::memory_order_acquire)) return false;

  // 2. Not idle if packets are currently traveling over the network
  if (pending_acks_.load(std::memory_order_acquire) > 0) return false;

  // 3. Not idle if any entity has data waiting to be pumped
  for (const Entity& entity : entities_) {
    if (entity.get_pending_flags() != 0) return false;
  }

  // Truly idle. Safe to tear down.
  return true;
}

// FreeRTOS requires a static C-style function signature. We use the handler_args
// to cast the pointer back into our specific C++ instance.
void MqttDevice::static_event_handler(void* handler_args, esp_event_base_t /*base*/,
                                      int32_t event_id, void* event_data) {
  MqttDevice* instance = static_cast<MqttDevice*>(handler_args);
  esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

  instance->handle_event(event_id, event);
}

void MqttDevice::pump_queue() {
  static uint16_t pump_count = 0;
  static bool was_disconnected = true;
  static bool have_logged_since_reconnected = false;

  // If we are offline, abort entirely. The flags stay safely set on the entities.
  if (!is_connected_.load(std::memory_order_acquire)) {
    was_disconnected = true;
    return;
  }

  if (was_disconnected) {
    char buf[128];
    get_unique_id(buf, "");
    ESP_LOGI(TAG, "Connected to broker as %s. Pumping %zu entities.", buf, entities_.count_items());
    was_disconnected = false;
    have_logged_since_reconnected = false;
    pump_count = 0;
  }

  ++pump_count;

  constexpr int MAX_IN_FLIGHT = 1;
  int msg_id = 0;
  topic_buf_t topic;
  topic[0] = '\0';

  static constinit sjson::StackBuffer<1024> buffer;

  for (Entity& entity : entities_) {
    buffer.reset();

    // Only block if the ACK window is full
    if (pending_acks_.load(std::memory_order_acquire) >= MAX_IN_FLIGHT) break;

    if (entity.get_pending_flags() & Entity::FLAG_DISCOVERY) {
      entity.get_discovery_topic(topic);
      entity.get_discovery_payload(buffer);
      msg_id = mqtt_enqueue(topic, buffer.c_str(), 1, 1);
      if (msg_id >= 0) {
        entity.clear_flag(Entity::FLAG_DISCOVERY);
        pending_acks_.fetch_add(1, std::memory_order_relaxed);
      }

    } else if (entity.get_pending_flags() & Entity::FLAG_SUBSCRIBE) {
      entity.get_command_topic(topic);
      msg_id = esp_mqtt_client_subscribe_single(client_, topic, 1);
      if (msg_id >= 0) {
        entity.clear_flag(Entity::FLAG_SUBSCRIBE);
        pending_acks_.fetch_add(1, std::memory_order_relaxed);
        ESP_LOGD(TAG, "Subscribed to topic: %s", topic);
      } else if (msg_id == -2) {
        ESP_LOGW(TAG, "Outbox full. Failed to subscribe to topic: %s", topic);
      } else if (msg_id == -1) {
        ESP_LOGW(TAG, "Failed to subscribe to topic: %s", topic);
      }

    } else if (entity.get_pending_flags() & Entity::FLAG_STATE) {
      entity.get_state_topic(topic);
      int qos = entity.get_state_qos();
      int retain = entity.get_state_retain();

      if (qos > 0) {
        // --- CRITICAL STATE (QoS 1) ---
        // Puts it in the sliding window. Must receive an ACK.
        msg_id = mqtt_enqueue(topic, entity.get_state_payload().c_str(), qos, retain);
        if (msg_id >= 0) {
          entity.clear_flag(Entity::FLAG_STATE);
          pending_acks_.fetch_add(1, std::memory_order_relaxed);
        }
        // If msg_id < 0, the flag remains set. An eventual ACK from another
        // in-flight packet or a reconnect event will re-trigger the pump.
      } else {
        // --- EPHEMERAL TELEMETRY (QoS 0) ---
        // Fire and forget.
        msg_id = mqtt_enqueue(topic, entity.get_state_payload().c_str(), 0, 0);

        // Unconditionally clear the flag to prevent deadlocks.
        // If the socket was full, this specific reading is dropped safely.
        entity.clear_flag(Entity::FLAG_STATE);
      }
    }
  }

  if (!have_logged_since_reconnected && is_idle()) {
    ESP_LOGI(TAG, "All entities pumped after %d pumps", int{pump_count});
    have_logged_since_reconnected = true;
  } else if (!is_idle()) {
    ESP_LOGD(TAG, "Pump %d complete. Pending ACKs: %d. Last topic: %s, last msg_id: %d",
             int{pump_count}, int{pending_acks_.load(std::memory_order_acquire)}, topic, msg_id);
  }
}

void MqttDevice::handle_event(int32_t event_id, esp_mqtt_event_handle_t event) {
  switch (event_id) {
    case MQTT_EVENT_ERROR:
      // Catch this quietly to stop the "Unhandled Event: 0" log spam.
      // It fires frequently when the underlying TCP socket resets or TLS fails.
      ESP_LOGW(TAG, "MQTT_EVENT_ERROR (Transport/Socket issue)");
      break;

    case MQTT_EVENT_BEFORE_CONNECT:
      // Safe to ignore, just noise.
      break;

    case MQTT_EVENT_CONNECTED:
      ESP_LOGD(TAG, "Connected to Broker.");
      // 1. Mark online to unpause the pump
      is_connected_.store(true, std::memory_order_release);
      // 2. Erase any stranded ACKs from packets lost during the outage
      pending_acks_.store(0, std::memory_order_release);
      // 3. Flag entities and start pumping
      on_connected();
      break;

    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGW(TAG, "Disconnected from Broker.");
      // Pause the pump. We don't touch pending_acks_ because the CONNECT event will clear it.
      is_connected_.store(false, std::memory_order_release);
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
    case MQTT_EVENT_DELETED:
      pending_acks_.fetch_sub(1, std::memory_order_release);
      pump_queue();
      break;

    default:
      ESP_LOGW(TAG, "Unhandled MQTT Event ID: %d", event_id);
      break;
  }
}

void MqttDevice::on_connected() {
  // 1. Flag everything for setup
  for (Entity& entity : entities_) {
    entity.request_discovery();
    entity.request_publish();  // Ensure the latest state is sent on boot
  }

  // 2. The entities now hold the lock via their bitfields.
  // We can safely release the global boot guard.
  initial_setup_complete_.store(true, std::memory_order_release);

  // 3. Kick off the first packet
  pump_queue();
}

int MqttDevice::mqtt_publish(const char* topic, const char* payload, int qos, int retain) {
  int msg_id = esp_mqtt_client_publish(client_, topic, payload, 0, qos, retain);
  if (msg_id >= 0) {
    ESP_LOGD(TAG, "Published message to topic: %s, qos: %d, retain: %d msgid: %d", topic, qos,
             retain, msg_id);
  } else if (msg_id == -2) {
    ESP_LOGW(TAG, "Outbox full. Failed to publish message to topic: %s", topic);
  } else if (msg_id == -1) {
    ESP_LOGW(TAG, "Failed to publish message to topic: %s", topic);
  }

  return msg_id;
}

int MqttDevice::mqtt_enqueue(const char* topic, const char* payload, int qos, int retain,
                             bool store) {
  int msg_id = esp_mqtt_client_enqueue(client_, topic, payload, 0, qos, retain, store);
  if (msg_id > 0) {
    ESP_LOGD(TAG, "Enqueued message to topic: %s, qos: %d, retain: %d msgid: %d", topic, qos,
             retain, msg_id);
  } else if (msg_id == -2) {
    ESP_LOGW(TAG, "Outbox full. Failed to enqueue message to topic: %s", topic);
  } else if (msg_id == -1) {
    ESP_LOGW(TAG, "Failed to enqueue message to topic: %s", topic);
  }
  return msg_id;
}

}  // namespace HAPPY::Transports