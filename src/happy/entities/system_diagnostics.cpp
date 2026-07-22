#include "happy/entities/system_diagnostics.hpp"

#include <driver/temperature_sensor.h>
#include <esp_app_desc.h>
#include <esp_heap_caps.h>
#include <esp_image_format.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <string>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <time.h>

#include "espbase/esp_result.hpp"

static std::string get_temperature_celsius() {
  static auto sensor = []() -> EspResult<temperature_sensor_handle_t> {
    temperature_sensor_handle_t temp_handle;
    temperature_sensor_config_t temp_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 50);
    temp_config.flags.allow_pd = 1;  // Allow power down mode for lower power consumption
    if (EspError err = temperature_sensor_install(&temp_config, &temp_handle)) {
      return err.log("TempSensor", "Failed to install temperature sensor");
    }
    if (EspError err = temperature_sensor_enable(temp_handle)) {
      return err.log("TempSensor", "Failed to enable temperature sensor");
    }
    return temp_handle;
  }();
  if (sensor) {
    float temp_c;
    if (temperature_sensor_get_celsius(*sensor, &temp_c) == ESP_OK) {
      return std::to_string(temp_c);
    }
  }
  return "unknown";
}

static const char* get_reset_reason_str() {
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:
      return "Unknown";
    case ESP_RST_POWERON:
      return "Power-on";
    case ESP_RST_EXT:
      return "External Pin";
    case ESP_RST_SW:
      return "Software Reset";
    case ESP_RST_PANIC:
      return "Software Panic";
    case ESP_RST_INT_WDT:
      return "Interrupt Watchdog";
    case ESP_RST_TASK_WDT:
      return "Task Watchdog";
    case ESP_RST_WDT:
      return "Other Watchdog";
    case ESP_RST_DEEPSLEEP:
      return "Deep Sleep Wakeup";
    case ESP_RST_BROWNOUT:
      return "Brownout";
    case ESP_RST_SDIO:
      return "SDIO Reset";
    case ESP_RST_USB:
      return "USB Peripheral";
    case ESP_RST_JTAG:
      return "JTAG Reset";
    case ESP_RST_EFUSE:
      return "eFuse Error";
    case ESP_RST_PWR_GLITCH:
      return "Power Glitch";
    case ESP_RST_CPU_LOCKUP:
      return "CPU Lockup";
    default:
      return "Unrecognized Code";
  }
}

static std::string get_firmware_size_once() {
  static std::string firmware_size = []() -> std::string {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running) return "unknown";

    esp_image_metadata_t data;
    const esp_partition_pos_t pos = {.offset = running->address, .size = running->size};
    if (esp_image_get_metadata(&pos, &data) == ESP_OK) {
      return std::to_string(data.image_len);
    }
    return "unknown";
  }();
  return firmware_size;
}

static std::string get_boot_time_iso() {
  time_t now;
  time(&now);

  // If timestamp is before 2020, NTP hasn't synced yet
  if (now < 1577836800) return "unknown";

  // Boot Time = Current UNIX Epoch - ESP32 Uptime Seconds
  int64_t uptime_sec = esp_timer_get_time() / 1000000ULL;
  time_t boot_time = now - uptime_sec;

  struct tm timeinfo;
  gmtime_r(&boot_time, &timeinfo);

  // HA expects UTC for absolute timestamps (denoted by 'Z')
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return std::string(buf);
}

static std::string get_ip_address() {
  // 1. Grab the default Wi-Fi Station network interface
  esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (!netif) {
    return "Not Connected";
  }

  // 2. Request the IP configuration struct for this interface
  esp_netif_ip_info_t ip_info;
  if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
    // 3. Convert the raw 32-bit IP into a human-readable string
    char ip_str[IP4ADDR_STRLEN_MAX];
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, IP4ADDR_STRLEN_MAX);
    return HAPPY::buf2str(ip_str);
  }

  return "0.0.0.0";
}

namespace HAPPY::Entities {

SystemDiagnostics::SystemDiagnostics(Device& device)
    : boot_time_(device, "boot_time", "Boot Time",
                 {
                     .device_class = "timestamp",
                     .icon = "mdi:clock-start",
                     .get_value = get_boot_time_iso,
                 }),
      reboot_reason_(device, "reboot_reason", "Reboot Reason",
                     {
                         .icon = "mdi:restart",
                         .get_value = get_reset_reason_str,
                     }),
      compile_date_(device, "compile_date", "Firmware Build",
                    {
                        .icon = "mdi:wrench-clock",
                        .get_value =
                            []() {
                              const esp_app_desc_t* desc = esp_app_get_description();
                              char buf[64];
                              snprintf(buf, sizeof(buf), "%.*s %.*s UTC", sizeof(desc->date),
                                       desc->date, sizeof(desc->time), desc->time);
                              return std::string(buf);
                            },
                    }),
      free_iram_(
          device, "free_iram", "Free Internal RAM",
          {
              .device_class = "data_size",
              .unit_of_measurement = "B",
              .icon = "mdi:memory",
              .get_value =
                  []() { return std::to_string(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)); },
          }),
      free_spiram_(
          device, "free_spiram", "Free External RAM",
          {
              .device_class = "data_size",
              .unit_of_measurement = "B",
              .icon = "mdi:expansion-card",
              .get_value =
                  []() { return std::to_string(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); },
          }),
      firmware_size_(device, "firmware_size", "App Image Size",
                     {
                         .device_class = "data_size",
                         .unit_of_measurement = "B",
                         .icon = "mdi:file-code-outline",
                         .get_value = get_firmware_size_once,
                     }),

      ota_partition_size_(device, "ota_partition_size", "OTA Partition Size",
                          {
                              .device_class = "data_size",
                              .unit_of_measurement = "B",
                              .icon = "mdi:folder-table",
                              .get_value = []() -> std::string {
                                const esp_partition_t* running = esp_ota_get_running_partition();
                                if (running) {
                                  return std::to_string(running->size);
                                }
                                return "unknown";
                              },
                          }),
      fs_used_space_(device, "fs_used_space", "Filesystem Used Space",
                     {
                         .device_class = "data_size",
                         .unit_of_measurement = "B",
                         .icon = "mdi:harddisk",
                         .get_value = []() -> std::string {
                           struct statvfs stat;
                           if (statvfs("/fs", &stat) == 0) {
                             size_t used = (stat.f_blocks - stat.f_bfree) * stat.f_frsize;
                             return std::to_string(used);
                           }
                           return "unknown";
                         },
                     }),
      ip_address_(device, "ip_address", "IP Address",
                  {.icon = "mdi:ip-network", .get_value = get_ip_address}),
      temperature_(device, "temperature", "Temperature (Celsius)",
                   {
                       .device_class = "temperature",
                       .unit_of_measurement = "°C",
                       .icon = "mdi:thermometer",
                       .get_value = get_temperature_celsius,
                   }) {
}

void SystemDiagnostics::publish_all() const {
  boot_time_.publish();
  reboot_reason_.publish();
  compile_date_.publish();
  free_iram_.publish();
  free_spiram_.publish();
  firmware_size_.publish();
  ota_partition_size_.publish();
  fs_used_space_.publish();
  ip_address_.publish();
  temperature_.publish();
}

}  // namespace HAPPY::Entities