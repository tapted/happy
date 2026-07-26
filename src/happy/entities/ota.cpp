#include "happy/entities/ota.hpp"

#include <esp_app_desc.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <string>
#include <string_view>

#include "espbase/json.hpp"
#include "espbase/nvs_store.hpp"
#include "espbase/trampoline.hpp"

namespace HAPPY::Entities {

static constexpr char TAG[] = "HAPPY_OTA";
static constexpr char NAMESPACE[] = "ha_ota";

static void perform_ota(const char* url, const char* new_version_str) {
  ESP_LOGI(TAG, "Downloading firmware: %s (version: %s)", url, new_version_str);

  esp_http_client_config_t ota_client_config{};
  ota_client_config.url = url;
  ota_client_config.keep_alive_enable = true;

  esp_https_ota_config_t ota_config{};
  ota_config.http_config = &ota_client_config;

  esp_err_t ret = esp_https_ota(&ota_config);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "OTA Success! Saving new version %s to NVS...", new_version_str);

    // Save the dynamic version so it survives the reboot
    auto store = NvsStore::open(NAMESPACE, NVS_READWRITE);
    if (store) {
      store->set_string("version", new_version_str);
      store->commit();
    }

    ESP_LOGI(TAG, "OTA Success! Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_restart();
  } else {
    ESP_LOGE(TAG, "OTA Failed: %s", esp_err_to_name(ret));
  }
}

// static
void OtaController::ota_step(EspTask<OtaController>& task) {
  OtaController* self = task.data();
  std::string host(self->server_url_.get_value());
  std::string proj(self->project_name_.get_value());
  if (host.empty() || proj.empty()) {
    ESP_LOGE(TAG, "Server or Project not configured.");
    return;
  }

  // 2. Clean URL construction using operator+ (No std::format, no snprintf)
  const char* scheme = host.find("://") != std::string::npos ? "" : "http://";
  const char* slash = host.ends_with('/') ? "" : "/";
  std::string manifest_url = scheme + host + slash + "manifest.json";
  ESP_LOGI(TAG, "Fetching manifest[%s] from: %s", proj.c_str(), manifest_url.c_str());

  esp_http_client_config_t config{};
  config.url = manifest_url.c_str();
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (EspError err = esp_http_client_open(client, 0)) {
    err.log(TAG, "Failed to open HTTP connection");
    esp_http_client_cleanup(client);
    return;
  }

  int content_length = esp_http_client_fetch_headers(client);
  if (content_length <= 0) {
    ESP_LOGE(TAG, "Invalid manifest size.");
    esp_http_client_cleanup(client);
    return;
  }

  // Dynamic Heap Buffer for the Manifest
  // Saves FreeRTOS task stack and automatically scales to any manifest size
  std::string json_buffer(content_length, '\0');
  int read_len = esp_http_client_read(client, json_buffer.data(), content_length);
  esp_http_client_cleanup(client);

  if (read_len <= 0) return;

  unique_cjson root_ptr{cJSON_ParseWithLength(json_buffer.data(), read_len)};
  JsonNodeView root(root_ptr.get());
  if (!root) {
    ESP_LOGE(TAG, "Failed to parse manifest.json");
    return;
  }

  JsonNodeView project_node = root[proj.c_str()];
  if (!project_node) {
    ESP_LOGW(TAG, "Project '%s' not found in manifest.", proj.c_str());
    return;
  }

  auto ver_opt = project_node["version"].as_string();
  auto img_opt = project_node["image"].as_string();

  if (ver_opt && img_opt) {
    if (*ver_opt != self->current_version_) {
      std::string new_version(*ver_opt);
      ESP_LOGI(TAG, "New version found! Upgrading from %s to %s", self->current_version_.c_str(),
               new_version.c_str());

      std::string bin_url = scheme + host + slash + std::string(*img_opt);
      perform_ota(bin_url.c_str(), new_version.c_str());
    }
  } else {
    ESP_LOGI(TAG, "Firmware is up to date (%s).", self->current_version_.c_str());
  }
}

OtaController::OtaController(Device& device, const char* base_version)
    : current_version_(base_version),
      server_url_(device, "ota_server", "OTA Server IP", {.icon = "mdi:server-network"}),
      // TODO: default this to esp_app_get_description()->project_name
      project_name_(device, "ota_project", "OTA Project Name",
                    {.icon = "mdi:application-brackets"}),
      update_btn_(device, "ota_trigger", "Check & Apply Update",
                  {
                      .icon = "mdi:cellphone-arrow-down",
                      .on_press = [this](const auto&) { ota_trigger(); },
                  }),
      current_version_sensor_(device, "current_version", "Running Firmware Version",
                              {
                                  .icon = "mdi:tag-check",
                                  .get_value = trampoline<&OtaController::get_current_version>(),
                              },
                              this),
      base_version_sensor_(
          device, "base_version", "Compiled Base Version",
          {
              .icon = "mdi:tag-outline",
              .get_value = [](void*) -> std::string { return esp_app_get_description()->version; },
          }) {
  // Load the dynamically installed version from NVS if it exists.
  auto store = NvsStore::open(NAMESPACE, NVS_READONLY);
  if (store) {
    char buf[64]{};
    if (store->get_string("version", buf, sizeof(buf))) {
      current_version_ = buf;  // Overwrite the base version.
    }
  }
  ESP_LOGI(TAG, "OTA Controller initialized. Current version: %s", current_version_.c_str());
}

void OtaController::ota_trigger() {
  ESP_LOGI("HAPPY_OTA", "Saw ota_trigger. Starting OTA task...");
  ota_task_
      .start(
          {
              .name = "ota_task",
              .stack_size = 4096,
              .prevent_light_sleep = true,
              .notify_if_started = false,  // Error if running.
          },
          this, &OtaController::ota_step)
      .log_error(TAG, "Failed to start OTA task (already running?)");
}

}  // namespace HAPPY::Entities