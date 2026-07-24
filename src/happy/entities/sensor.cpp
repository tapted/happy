#include "happy/entities/sensor.hpp"

#include "espbase/json.h"

namespace HAPPY::Entities {

void Sensor::initialize_topics() {
  initialize_base_topics(false);
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