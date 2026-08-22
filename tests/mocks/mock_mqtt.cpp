#include "mocks/mock_mqtt.hpp"

#include "espbase/stdlib_main_loop.hpp"
#include "happy/transports/mqtt_device.hpp"

namespace HAPPY::Test {

MockMqtt& MockMqtt::instance() {
  static MockMqtt instance;
  return instance;
}

void MockMqtt::reset() {
  enqueue_calls_.clear();
  subscribe_calls_.clear();
  publish_calls_.clear();
  pending_acks_.clear();
  next_msg_id_ = 1;
  registered_handler_ = nullptr;
  registered_handler_arg_ = nullptr;
}

void MockMqtt::simulate_connect() {
  if (registered_handler_) {
    esp_mqtt_event_t event = {};
    event.event_id = MQTT_EVENT_CONNECTED;
    registered_handler_(registered_handler_arg_, "MQTT_EVENT", MQTT_EVENT_CONNECTED, &event);
    main_loop.wait_idle();
  }
}

void MockMqtt::simulate_disconnect() {
  if (registered_handler_) {
    esp_mqtt_event_t event = {};
    event.event_id = MQTT_EVENT_DISCONNECTED;
    registered_handler_(registered_handler_arg_, "MQTT_EVENT", MQTT_EVENT_DISCONNECTED, &event);
    main_loop.wait_idle();
  }
}

void MockMqtt::simulate_ack_next_pending() {
  if (pending_acks_.empty() || !registered_handler_) return;

  PendingAck ack = pending_acks_.front();
  pending_acks_.erase(pending_acks_.begin());

  esp_mqtt_event_t event = {};
  event.msg_id = ack.msg_id;
  event.event_id = ack.is_subscribe ? MQTT_EVENT_SUBSCRIBED : MQTT_EVENT_PUBLISHED;

  registered_handler_(registered_handler_arg_, "MQTT_EVENT", event.event_id, &event);
}

void MockMqtt::pump_until_idle_or_max(HAPPY::Transports::MqttDevice& device, int max_iterations) {
  int iterations = 0;
  main_loop.wait_idle();
  while (!device.is_idle() && iterations < max_iterations) {
    if (!pending_acks_.empty()) {
      simulate_ack_next_pending();
      main_loop.wait_idle();
    } else {
      device.poke();
      main_loop.wait_idle();
    }
    ++iterations;
  }
}

esp_mqtt_client_handle_t MockMqtt::mock_init(const esp_mqtt_client_config_t* /*config*/) {
  return active_client_;
}

esp_err_t MockMqtt::mock_destroy(esp_mqtt_client_handle_t /*client*/) {
  return ESP_OK;
}

esp_err_t MockMqtt::mock_start(esp_mqtt_client_handle_t /*client*/) {
  return ESP_OK;
}

esp_err_t MockMqtt::mock_stop(esp_mqtt_client_handle_t /*client*/) {
  return ESP_OK;
}

esp_err_t MockMqtt::mock_register_event(esp_mqtt_client_handle_t /*client*/, int32_t /*event_id*/,
                                        esp_event_handler_t event_handler,
                                        void* event_handler_arg) {
  registered_handler_ = event_handler;
  registered_handler_arg_ = event_handler_arg;
  return ESP_OK;
}

int MockMqtt::mock_publish(esp_mqtt_client_handle_t /*client*/, const char* topic, const char* data,
                           int len, int qos, int retain) {
  int msg_id = next_msg_id_++;
  std::string payload_str =
      (data && len > 0) ? std::string(data, len) : (data ? std::string(data) : "");
  publish_calls_.push_back({topic ? topic : "", payload_str, qos, retain, msg_id});
  return msg_id;
}

int MockMqtt::mock_enqueue(esp_mqtt_client_handle_t /*client*/, const char* topic, const char* data,
                           int len, int qos, int retain, bool store) {
  int msg_id = next_msg_id_++;
  std::string payload_str =
      (data && len > 0) ? std::string(data, len) : (data ? std::string(data) : "");
  enqueue_calls_.push_back({topic ? topic : "", payload_str, qos, retain, store, msg_id});
  if (qos > 0) {
    pending_acks_.push_back({false, msg_id});
  }
  return msg_id;
}

int MockMqtt::mock_subscribe_single(esp_mqtt_client_handle_t /*client*/, const char* topic,
                                    int qos) {
  int msg_id = next_msg_id_++;
  subscribe_calls_.push_back({topic ? topic : "", qos, msg_id});
  pending_acks_.push_back({true, msg_id});
  return msg_id;
}

}  // namespace HAPPY::Test

// --- C Function Implementations ---

extern "C" {

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t* config) {
  return HAPPY::Test::MockMqtt::instance().mock_init(config);
}

esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client) {
  return HAPPY::Test::MockMqtt::instance().mock_destroy(client);
}

esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) {
  return HAPPY::Test::MockMqtt::instance().mock_start(client);
}

esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client) {
  return HAPPY::Test::MockMqtt::instance().mock_stop(client);
}

esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, int32_t event_id,
                                         esp_event_handler_t event_handler,
                                         void* event_handler_arg) {
  return HAPPY::Test::MockMqtt::instance().mock_register_event(client, event_id, event_handler,
                                                               event_handler_arg);
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data,
                            int len, int qos, int retain) {
  return HAPPY::Test::MockMqtt::instance().mock_publish(client, topic, data, len, qos, retain);
}

int esp_mqtt_client_enqueue(esp_mqtt_client_handle_t client, const char* topic, const char* data,
                            int len, int qos, int retain, bool store) {
  return HAPPY::Test::MockMqtt::instance().mock_enqueue(client, topic, data, len, qos, retain,
                                                        store);
}

int esp_mqtt_client_subscribe_single(esp_mqtt_client_handle_t client, const char* topic, int qos) {
  return HAPPY::Test::MockMqtt::instance().mock_subscribe_single(client, topic, qos);
}
}
