#include "happy/device.hpp"

#include <esp_log.h>

#include "espbase/json.h"
#include "happy/entity.hpp"

namespace HAPPY {

void Device::register_entity(HAPPY::Entity* entity) {
  entities_.push_back(entity);
}

void Device::begin() {
  for (Entity& entity : entities_) {
    entity.initialize_topics();
  }
}

void Device::dispatch_command(std::string_view topic, std::string_view payload) const {
  for (Entity& entity : entities_) {
    if (!entity.get_command_topic().empty() && entity.get_command_topic() == topic) {
      // Parse JSON exactly ONCE, passing the RAII wrapper to the virtual method
      unique_cjson root{cJSON_ParseWithLength(payload.data(), payload.length())};
      if (root) {
        entity.handle_command(root);
      } else {
        ESP_LOGE("HA_DEVICE", "Failed to parse command payload for %s", topic.data());
      }
      return;  // Stop searching once routed
    }
  }
}

}  // namespace HAPPY