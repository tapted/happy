#pragma once

#include <mqtt_client.h>
#include <string>
#include <vector>

namespace HAPPY::Transports {
class MqttDevice;
}

namespace HAPPY::Test {

struct EnqueueCall {
  std::string topic;
  std::string payload;
  int qos;
  int retain;
  bool store;
  int msg_id;
};

struct SubscribeCall {
  std::string topic;
  int qos;
  int msg_id;
};

struct PublishCall {
  std::string topic;
  std::string payload;
  int qos;
  int retain;
  int msg_id;
};

class MockMqtt {
 public:
  static MockMqtt& instance();

  void reset();

  const std::vector<EnqueueCall>& enqueue_calls() const { return enqueue_calls_; }
  const std::vector<SubscribeCall>& subscribe_calls() const { return subscribe_calls_; }
  const std::vector<PublishCall>& publish_calls() const { return publish_calls_; }

  void simulate_connect();
  void simulate_disconnect();
  void simulate_ack_next_pending();
  void pump_until_idle_or_max(HAPPY::Transports::MqttDevice& device, int max_iterations = 100);

  // Internal C mock interface hooks
  esp_mqtt_client_handle_t mock_init(const esp_mqtt_client_config_t* config);
  esp_err_t mock_destroy(esp_mqtt_client_handle_t client);
  esp_err_t mock_start(esp_mqtt_client_handle_t client);
  esp_err_t mock_stop(esp_mqtt_client_handle_t client);
  esp_err_t mock_register_event(esp_mqtt_client_handle_t client, int32_t event_id,
                                esp_event_handler_t event_handler, void* event_handler_arg);
  int mock_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len,
                   int qos, int retain);
  int mock_enqueue(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len,
                   int qos, int retain, bool store);
  int mock_subscribe_single(esp_mqtt_client_handle_t client, const char* topic, int qos);

 private:
  MockMqtt() = default;

  std::vector<EnqueueCall> enqueue_calls_;
  std::vector<SubscribeCall> subscribe_calls_;
  std::vector<PublishCall> publish_calls_;

  int next_msg_id_ = 1;
  esp_mqtt_client_handle_t active_client_ = reinterpret_cast<esp_mqtt_client_handle_t>(0x12345678);
  esp_event_handler_t registered_handler_ = nullptr;
  void* registered_handler_arg_ = nullptr;

  struct PendingAck {
    bool is_subscribe;
    int msg_id;
  };
  std::vector<PendingAck> pending_acks_;
};

}  // namespace HAPPY::Test
