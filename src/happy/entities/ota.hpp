#pragma once

#include "espbase/esp_task.hpp"
#include "happy/entities/button.hpp"
#include "happy/entities/text.hpp"
#include "happy/entities/sensor.hpp"

namespace HAPPY::Entities {

class OtaController {
 public:
  OtaController(Device& device, const char* base_version);

  std::string get_current_version() const { return current_version_; }

 private:
  void ota_trigger();
  static void ota_step(EspTask<OtaController>& task);

  std::string current_version_;
  static constexpr const char* TAG = "HAPPY_OTA";

  Text server_url_;
  Text project_name_;
  Button update_btn_;
  Sensor current_version_sensor_;
  Sensor base_version_sensor_;
  EspTask<OtaController> ota_task_;
};

}  // namespace HAPPY::Entities