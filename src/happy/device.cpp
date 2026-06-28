#include "happy/device.hpp"

#include <esp_log.h>

#include "espbase/json.h"
#include "happy/entity.hpp"

namespace HAPPY {

// Injects the HA "device" grouping block into an existing json.h builder
void Device::inject_into(JsonObjectBuilder& builder) const {
  builder.with_object("device", [this](auto& dev) {
    dev.with_array("identifiers", [this](auto& arr) { arr.push(config_.identifiers); });
    dev.set("name", config_.name);

    if (config_.manufacturer) dev.set("manufacturer", config_.manufacturer);
    if (config_.model) dev.set("model", config_.model);
    if (config_.sw_version) dev.set("sw_version", config_.sw_version);
  });
}

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
      entity.handle_command(payload);
      return;  // Stop searching once routed
    }
  }
}

}  // namespace HAPPY