#include "happy/entities/alarm.hpp"

#include <format>

static constexpr const char* const ALARM_TONES[] = {
    "acknowledge", "success", "error", "startup", "Jasmine Flower",
};

namespace HAPPY::Entities {

AlarmController::AlarmController(Device& device, uint8_t alarm_id, OnAlarmUpdateCallback on_update,
                                 OnAlarmUpdateCallback on_test)
    : id(alarm_id),
      on_update_(on_update),
      on_test_(on_test),

      // We dynamically construct the IDs like "alarm_1_time"
      // TODO: is this bad? time_,tone_,test_btn_ take std::string_view so we could _maybe_ pass
      // a ref to a char[] buffer.
      time_id_(std::format("alarm_{}_time", id)),
      time_name_(std::format("Alarm {} Time", id)),
      tone_id_(std::format("alarm_{}_tone", id)),
      tone_name_(std::format("Alarm {} Tone", id)),
      test_id_(std::format("alarm_{}_test", id)),
      test_name_(std::format("Test Alarm {}", id)),

      // Initialize the entities directly attached to the device registry
      time_(device, time_id_, time_name_,
            {
                .on_update = [this](const auto& t) { this->on_update_(*this); },
            }),

      tone_(device, tone_id_, tone_name_,
            {
                .icon = "mdi:music-note",
                .options = ALARM_TONES,
                .on_update = [this](const auto& t) { this->on_update_(*this); },
            }),

      test_btn_(device, test_id_, test_name_,
                {
                    .icon = "mdi:play-circle-outline",
                    .on_press = [this](const auto& btn) { this->on_test_(*this); },
                }) {
}

}  // namespace HAPPY::Entities