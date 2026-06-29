#include "happy/entities/system_diagnostics.hpp"

#include <esp_app_desc.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>

// --- ESP-IDF Data Fetchers ---
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

namespace HAPPY::Entities {

SystemDiagnostics::SystemDiagnostics(Device& device)
    : boot_time_(device, "boot_time", "Boot Time",
                 {
                     .device_class = "timestamp",
                     .icon = "mdi:clock-start",
                     .get_value = []() { return get_boot_time_iso(); },
                 }),
      reboot_reason_(device, "reboot_reason", "Reboot Reason",
                     {
                         .icon = "mdi:restart",
                         .get_value = []() { return get_reset_reason_str(); },
                     }),
      compile_date_(device, "compile_date", "Firmware Build",
                    {
                        .icon = "mdi:wrench-clock",
                        .get_value =
                            []() {
                              const esp_app_desc_t* desc = esp_app_get_description();
                              char buf[64];
                              snprintf(buf, sizeof(buf), "%s %s UTC", desc->date, desc->time);
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
          }) {
}

void SystemDiagnostics::publish_all() const {
  boot_time_.publish();
  reboot_reason_.publish();
  compile_date_.publish();
  free_iram_.publish();
  free_spiram_.publish();
}

}  // namespace HAPPY::Entities