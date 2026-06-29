#pragma once

#include "happy/device.hpp"
#include "happy/entities/sensor.hpp"

namespace HAPPY::Entities {

class SystemDiagnostics {
 public:
  explicit SystemDiagnostics(Device& device);

  // Push all sensor states to MQTT
  void publish_all() const;

 private:
  Sensor boot_time_;
  Sensor reboot_reason_;
  Sensor compile_date_;
  Sensor free_iram_;
  Sensor free_spiram_;
};

}  // namespace HAPPY::Entities