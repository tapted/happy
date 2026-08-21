#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_mqtt_client* esp_mqtt_client_handle_t;

typedef enum {
  MQTT_EVENT_ANY = -1,
  MQTT_EVENT_ERROR = 0,
  MQTT_EVENT_CONNECTED,
  MQTT_EVENT_DISCONNECTED,
  MQTT_EVENT_SUBSCRIBED,
  MQTT_EVENT_UNSUBSCRIBED,
  MQTT_EVENT_PUBLISHED,
  MQTT_EVENT_DATA,
  MQTT_EVENT_BEFORE_CONNECT,
  MQTT_EVENT_DELETED,
  MQTT_USER_EVENT,
} esp_mqtt_event_id_t;

typedef struct esp_mqtt_event_t {
  esp_mqtt_event_id_t event_id;
  const char* topic;
  int topic_len;
  const char* data;
  int data_len;
  int msg_id;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t* esp_mqtt_event_handle_t;

#ifndef ESP_EVENT_BASE_TYPEDEF
#define ESP_EVENT_BASE_TYPEDEF
typedef const char* esp_event_base_t;
#endif

typedef void (*esp_event_handler_t)(void* event_handler_arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);

typedef struct esp_mqtt_client_config_t {
  struct {
    struct {
      const char* uri;
    } address;
  } broker;
  struct {
    const char* username;
    struct {
      const char* password;
    } authentication;
  } credentials;
} esp_mqtt_client_config_t;

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t* config);
esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, int32_t event_id,
                                         esp_event_handler_t event_handler,
                                         void* event_handler_arg);

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data,
                            int len, int qos, int retain);
int esp_mqtt_client_enqueue(esp_mqtt_client_handle_t client, const char* topic, const char* data,
                            int len, int qos, int retain, bool store);
int esp_mqtt_client_subscribe_single(esp_mqtt_client_handle_t client, const char* topic, int qos);

#ifdef __cplusplus
}
#endif
