#pragma once

#include "happy/entities/lazy_sensor.hpp"

namespace HAPPY::Entities {

class SystemDiagnostics {
 public:
  explicit SystemDiagnostics(Device& device);

  // Push all sensor states to MQTT that can change. Immutable sensors (boot time, compile date,
  // reboot reason) rely on the device to call request_publish() whenever the mqtt connection is
  // established. (IP Address can change but counts as immutable for this purpose.)
  void publish_all_mutable(bool is_time_sync = false);

 private:
  Sensor boot_time_;
  Sensor reboot_reason_;
  Sensor compile_date_;

  Sensor free_iram_at_boot_;
  PollingSensor<size_t> free_iram_;
  PollingSensor<size_t> free_spiram_;

  CachingConstSensor<size_t> firmware_size_;

  PollingSensor<size_t> fs_used_space_;
  Sensor ip_address_;
  PollingSensor<float> temperature_;
};

}  // namespace HAPPY::Entities