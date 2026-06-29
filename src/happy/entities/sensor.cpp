#include "happy/entities/sensor.hpp"

#include "espbase/json.h"

namespace HAPPY::Entities {

void Sensor::initialize_topics() {
  initialize_base_topics(false);

  // Override the shared state_topic to be isolated for this specific sensor
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%s/%.*s/state", device_.get_identifier(),
                static_cast<int>(object_id_.length()), object_id_.data());
  state_topic_ = buf;
}

std::string Sensor::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  // Ensure the discovery points to our isolated topic
  builder.set("state_topic", state_topic_.c_str());

  if (config_.device_class) builder.set("device_class", config_.device_class);
  if (config_.unit_of_measurement) builder.set("unit_of_measurement", config_.unit_of_measurement);
  if (config_.icon) builder.set("icon", config_.icon);
  if (config_.entity_category) builder.set("entity_category", config_.entity_category);

  return doc.to_string();
}

}  // namespace HAPPY::Entities