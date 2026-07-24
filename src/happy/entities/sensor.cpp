#include "happy/entities/sensor.hpp"

#include "espbase/json.h"

namespace HAPPY::Entities {

std::string Sensor::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  // Ensure the discovery points to our isolated topic
  topic_buf_t state_topic;
  get_state_topic(state_topic);
  builder.set("state_topic", (const char*)state_topic);

  if (config_.device_class) builder.set("device_class", config_.device_class);
  if (config_.unit_of_measurement) builder.set("unit_of_measurement", config_.unit_of_measurement);
  if (config_.icon) builder.set("icon", config_.icon);
  if (config_.entity_category) builder.set("entity_category", config_.entity_category);

  return doc.to_string();
}

}  // namespace HAPPY::Entities