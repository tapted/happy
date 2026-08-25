#include "happy/entities/sensor.hpp"

#include <esp_log.h>
#include <stddef.h>

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Sensor::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;                                      // Max 32 entries.
  auto doc = stack_json(node_if("device_class", config_.device_class),  //
                        node_if("unit_of_measurement", config_.unit_of_measurement),  //
                        node_if("icon", config_.icon),                                //
                        node_if("entity_category", config_.entity_category));
  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

size_t Sensor::get_state_payload(sjson::Buffer& buffer) {
  size_t written = config_.get_value(user_ctx, buffer);
  if (config_.on_state_publish) {
    config_.on_state_publish(*this, buffer.last_write(written));
  }
  return written;
}

}  // namespace HAPPY::Entities