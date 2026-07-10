#include "happy/entities/alarm.hpp"

namespace HAPPY::Entities {

static std::string to_id(const char* prefix, uint8_t alarm_id, const char* suffix, char sep = '_') {
  char buffer[32];
  if (suffix[0] == '\0') {
    std::snprintf(buffer, sizeof(buffer), "%s%c%d", prefix, sep, alarm_id);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%s%c%d%c%s", prefix, sep, alarm_id, sep, suffix);
  }
  return std::string(buffer);
}

AlarmController::AlarmController(Device& device, uint8_t alarm_id,
                                 std::span<const char* const> alarm_tones,
                                 OnAlarmUpdateCallback on_update, OnAlarmUpdateCallback on_test)
    : id(alarm_id),
      on_update_(on_update),
      on_test_(on_test),

      // We dynamically construct the IDs like "alarm_1_time"
      // TODO: is this bad? time_,tone_,test_btn_ take std::string_view so we could _maybe_ pass
      // a ref to a char[] buffer.
      time_id_(to_id("alarm", id, "time")),
      time_name_(to_id("Alarm", id, "Time", ' ')),
      tone_id_(to_id("alarm", id, "tone")),
      tone_name_(to_id("Alarm", id, "Tone", ' ')),
      test_id_(to_id("alarm", id, "test")),
      test_name_(to_id("Test Alarm", id, "", ' ')),

      // Initialize the entities directly attached to the device registry
      time_(device, time_id_, time_name_,
            {
                .on_update = [this](const auto&) { this->on_update_(*this); },
            }),

      tone_(device, tone_id_, tone_name_,
            {
                .icon = "mdi:music-note",
                .options = alarm_tones,
                .on_update = [this](const auto&) { this->on_update_(*this); },
            }),

      test_btn_(device, test_id_, test_name_,
                {
                    .icon = "mdi:play-circle-outline",
                    .on_press = [this](const auto&) { this->on_test_(*this); },
                }) {
}

}  // namespace HAPPY::Entities