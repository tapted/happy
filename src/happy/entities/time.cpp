#include "happy/entities/time.hpp"

#include <charconv>

#include "espbase/json.h"

namespace HAPPY::Entities {

void Time::initialize_topics() {
  initialize_base_topics(true);
}

std::string Time::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  builder.set("command_topic", command_topic_.c_str());
  if (config_.icon) builder.set("icon", config_.icon);

  return doc.to_string();
}

std::string Time::get_state_payload() const {
  char buffer[12];  // "HH:MM:SS" + null terminator
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour(), minute(), second());
  return std::string(buffer);
}

void Time::handle_command(std::string_view payload) {
  // Payload is "HH:MM" or "HH:MM:SS"
  if (payload.length() >= 5) {
    std::from_chars(payload.data(), payload.data() + 2, state_.hour_);
    std::from_chars(payload.data() + 3, payload.data() + 5, state_.minute_);

    if (payload.length() >= 8) {
      std::from_chars(payload.data() + 6, payload.data() + 8, state_.second_);
    } else {
      state_.second_ = 0;
    }

    save_state();
    on_change();
    device_.publish(*this);
  }
}

}  // namespace HAPPY::Entities