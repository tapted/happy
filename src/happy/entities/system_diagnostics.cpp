#include "happy/entities/system_diagnostics.hpp"

#include <driver/temperature_sensor.h>
#include <esp_app_desc.h>
#include <esp_heap_caps.h>
#include <esp_image_format.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <time.h>

#include "espbase/esp_result.hpp"

extern size_t system_diagnostic_free_iram_at_boot;
size_t system_diagnostic_free_iram_at_boot = 0;

static constexpr time_t TIME_SYNCED_THRESHOLD = 1577836800;  // Jan 1, 2020 00:00:00 UTC

static float get_temperature_celsius() {
  static auto sensor = []() -> EspResult<temperature_sensor_handle_t> {
    temperature_sensor_handle_t temp_handle;
    temperature_sensor_config_t temp_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 50);
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
      return temp_c;
    }
  }
  return 0.0f;
}

static std::string get_reset_reason_str(void*) {
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:
      return "ESP_RST_UNKNOWN";
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
      return "Undefined";
  }
}

static size_t get_firmware_size() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (!running) return 0;

  esp_image_metadata_t data;
  const esp_partition_pos_t pos = {.offset = running->address, .size = running->size};
  if (esp_image_get_metadata(&pos, &data) == ESP_OK) {
    return data.image_len;
  }
  return 0;
}

static std::string get_boot_time_iso(void*) {
  time_t now;
  time(&now);

  // If timestamp is before 2020, NTP hasn't synced yet
  if (now < TIME_SYNCED_THRESHOLD) return "unknown";

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

static std::string get_ip_address(void*) {
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

static constexpr Sensor::Config boot_time = {
    .device_class = "timestamp",
    .icon = "mdi:clock-start",
    .get_value = get_boot_time_iso,
};

static constexpr Sensor::Config reboot_reason = {
    .icon = "mdi:restart",
    .get_value = get_reset_reason_str,
};

static constexpr Sensor::Config compile_date = {
    .icon = "mdi:wrench-clock",
    .get_value =
        [](void*) {
          const esp_app_desc_t* desc = esp_app_get_description();
          char buf[64];
          snprintf(buf, sizeof(buf), "%.*s %.*s UTC", sizeof(desc->date), desc->date,
                   sizeof(desc->time), desc->time);
          return std::string(buf);
        },
};

static constexpr Sensor::Config ip_address = {
    .icon = "mdi:ip-network",
    .get_value = get_ip_address,
};

static constexpr Sensor::Config free_iram_at_boot = {
    .device_class = "data_size",
    .unit_of_measurement = "B",
    .icon = "mdi:memory-arrow-down",
    .get_value = [](void*) { return std::to_string(system_diagnostic_free_iram_at_boot); },
};

SystemDiagnostics::SystemDiagnostics(Device& device)
    : boot_time_(device, "boot_time", "Boot Time", boot_time),
      reboot_reason_(device, "reboot_reason", "Reboot Reason", reboot_reason),
      compile_date_(device, "compile_date", "Firmware Build", compile_date),
      free_iram_at_boot_(device, "free_iram_at_boot", "Free IRAM at Boot", free_iram_at_boot),
      free_iram_(device, "free_iram", "Free Internal RAM",
                 {.device_class = "data_size", .unit_of_measurement = "B", .icon = "mdi:memory"},
                 []() -> size_t { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }),

      free_spiram_(
          device, "free_spiram", "Free External RAM",
          {.device_class = "data_size", .unit_of_measurement = "B", .icon = "mdi:expansion-card"},
          []() -> size_t { return heap_caps_get_free_size(MALLOC_CAP_SPIRAM); }),

      firmware_size_(device, "firmware_size", "App Image Size",
                     {.device_class = "data_size",
                      .unit_of_measurement = "B",
                      .icon = "mdi:file-code-outline"},
                     get_firmware_size),

      ota_partition_size_(
          device, "ota_partition_size", "OTA Partition Size",
          {.device_class = "data_size", .unit_of_measurement = "B", .icon = "mdi:folder-table"},
          []() -> size_t {
            const esp_partition_t* running = esp_ota_get_running_partition();
            return running ? running->size : 0;
          }),

      fs_used_space_(
          device, "fs_used_space", "Filesystem Used Space",
          {.device_class = "data_size", .unit_of_measurement = "B", .icon = "mdi:harddisk"},
          []() -> size_t {
            struct statvfs stat;
            if (statvfs("/fs", &stat) == 0) {
              return (stat.f_blocks - stat.f_bfree) * stat.f_frsize;
            }
            return 0;
          }),

      ip_address_(device, "ip_address", "IP Address", ip_address),

      temperature_(
          device, "temperature", "Temperature",
          {.device_class = "temperature", .unit_of_measurement = "°C", .icon = "mdi:thermometer"},
          get_temperature_celsius) {
}

void SystemDiagnostics::publish_all_mutable(bool is_time_sync) {
  free_iram_.publish_if_changed();
  free_spiram_.publish_if_changed();
  fs_used_space_.publish_if_changed();
  temperature_.publish_if_changed();

  static bool boot_time_published = false;
  if (is_time_sync && !boot_time_published) {
    // Special case: Boot Time is only published once, after NTP has synced.
    time_t now;
    time(&now);
    if (now >= TIME_SYNCED_THRESHOLD) {
      boot_time_.request_publish();
      boot_time_published = true;
    }
  }
}

}  // namespace HAPPY::Entities