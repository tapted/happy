#pragma once

#include "happy/entities/button.hpp"
#include "happy/entities/select.hpp"
#include "happy/entities/time.hpp"

namespace HAPPY::Entities {

// An opinionated composite entity that represents a single alarm with time, tone, and test button.
// E.g.,
// ```cpp
// new HAPPY::Entities::AlarmController(
//     dongley_device, 1,
//     [](const HAPPY::Entities::AlarmController& alarm) {
//       ESP_LOGI(TAG, "Alarm %d updated: time=%02d:%02d:%02d, tone=%s", alarm.id,
//                alarm.time().hour(), alarm.time().minute(), alarm.time().second(),
//                alarm.selected_tone().data());
//     },
//     [](const HAPPY::Entities::AlarmController& alarm) {
//       const auto tone = alarm.selected_tone();
//       ESP_LOGI(TAG, "Alarm %d test triggered!", alarm.id);
//       if (tone == "acknowledge") {
//         HAL::Passive::default_instance().play(HAL::beeps::acknowledge);
//       } else if (tone == "success") {
//         HAL::Passive::default_instance().play(HAL::beeps::success);
//       } else if (tone == "error") {
//         HAL::Passive::default_instance().play(HAL::beeps::error);
//       } else if (tone == "startup") {
//         HAL::Passive::default_instance().play(HAL::beeps::startup);
//       } else if (tone == "Jasmine Flower") {
//         HAL::Passive::default_instance().play(HAL::melodies::mo_li_hua);
//       }
//     });
// ```
class AlarmController {
 public:
  const uint8_t id;

  using OnAlarmUpdateCallback = std::function<void(const AlarmController&)>;

  AlarmController(Device& device, uint8_t alarm_id, OnAlarmUpdateCallback on_update,
                  OnAlarmUpdateCallback on_test);

  std::string_view selected_tone() const { return tone_.get_selected(); }
  const Time& time() const { return time_; }

 private:
  OnAlarmUpdateCallback on_update_;
  OnAlarmUpdateCallback on_test_;

  // We store the strings here so the std::string_views in the Entities stay valid
  std::string time_id_, time_name_;
  std::string tone_id_, tone_name_;
  std::string test_id_, test_name_;

  Time time_;
  Select tone_;
  Button test_btn_;
};

}  // namespace HAPPY::Entities