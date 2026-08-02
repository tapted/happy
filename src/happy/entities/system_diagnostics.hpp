#pragma once

#include "happy/entities/lazy_sensor.hpp"

namespace HAPPY::Entities {

class SystemDiagnostics {
 public:
  explicit SystemDiagnostics(Device& device);

  // Push all sensor states to MQTT
  void publish_all();

 private:
  Sensor boot_time_;
  Sensor reboot_reason_;
  Sensor compile_date_;

  PollingSensor<size_t> free_iram_;
  PollingSensor<size_t> free_spiram_;

  CachingConstSensor<size_t> firmware_size_;
  CachingConstSensor<size_t> ota_partition_size_;

  PollingSensor<size_t> fs_used_space_;
  Sensor ip_address_;
  PollingSensor<float> temperature_;
};

}  // namespace HAPPY::Entities