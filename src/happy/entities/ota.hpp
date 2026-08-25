#pragma once

#include "espbase/esp_task.hpp"
#include "espbase/stack_json/buffer.hpp"
#include "happy/entities/button.hpp"
#include "happy/entities/sensor.hpp"
#include "happy/entities/text.hpp"

namespace HAPPY::Entities {

class OtaController {
 public:
  OtaController(Device& device, const char* base_version);

  size_t get_current_version(sjson::Buffer& buffer) const { return buffer.write(current_version_); }

 private:
  void ota_trigger(const Button&);
  static void ota_step(EspTask<OtaController>& task);

  std::string current_version_;

  Text server_url_;
  Text project_name_;
  Button update_btn_;
  Sensor current_version_sensor_;
  Sensor base_version_sensor_;
  EspTask<OtaController> ota_task_;
};

}  // namespace HAPPY::Entities