#pragma once

#include <ctime>
#include <string_view>

#include "happy/entities/text.hpp"

namespace HAPPY::Entities {

class DayMask : public TextT<8> {
 public:
  DayMask(Device& device, const char* object_id, const char* name, TextBase::Config config,
          void* ctx = nullptr)
      : TextT<8>(device, object_id, name, patch_config(config), "SMTWTFS", ctx) {}

  // Intercept the MQTT command to sanitize it before saving
  void handle_command(std::string_view payload) override;

  // Checks if the day specified in the tm struct is enabled
  bool is_set(const struct tm& timeinfo) const {
    return (get_bitmask() & (1 << timeinfo.tm_wday)) != 0;
  }

  // Returns the bitmask representing the enabled days. Bit 0 corresponds to Sunday, bit 1 to
  // Monday, and so on.
  uint8_t get_bitmask() const;

 private:
  TextBase::Config patch_config(TextBase::Config config) {
    if (config.icon == TextBase::Config{}.icon) config.icon = "mdi:calendar-range";
    return config;
  }
};

}  // namespace HAPPY::Entities