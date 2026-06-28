#include "happy/entities/time.hpp"

#include <charconv>
#include <format>

#include "espbase/json.h"

namespace HAPPY::Entities {

std::string Time::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  builder.set("command_topic", command_topic_.c_str());
  if (config_.icon) builder.set("icon", config_.icon);

  return doc.to_string();
}

std::string Time::get_state_payload() const {
  return std::format("{:02}:{:02}:{:02}", hour_, minute_, second_);
}

void Time::handle_command(std::string_view payload) {
  // Payload is "HH:MM" or "HH:MM:SS"
  if (payload.length() >= 5) {
    std::from_chars(payload.data(), payload.data() + 2, hour_);
    std::from_chars(payload.data() + 3, payload.data() + 5, minute_);

    if (payload.length() >= 8) {
      std::from_chars(payload.data() + 6, payload.data() + 8, second_);
    } else {
      second_ = 0;
    }

    if (config_.on_update) config_.on_update(*this);
    device_.publish(*this);
  }
}

}  // namespace HAPPY::Entities