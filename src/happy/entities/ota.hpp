#pragma once

#include "espbase/esp_task.hpp"
#include "happy/entities/button.hpp"
#include "happy/entities/text.hpp"

namespace HAPPY::Entities {

class OtaController {
 public:
  OtaController(Device& device, const char* current_version);

 private:
  void ota_trigger();
  static void ota_step(EspTask<OtaController>& task);

  const char* current_version_;
  static constexpr const char* TAG = "HAPPY_OTA";

  Text server_url_;
  Text project_name_;
  Button update_btn_;
  EspTask<OtaController> ota_task_;
};

}  // namespace HAPPY::Entities