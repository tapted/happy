#pragma once

#include <string>
#include <string_view>

#include "espbase/json_fwd.h"
#include "happy/core/intrusive_list.hpp"
#include "happy/device.hpp"

namespace HAPPY {

class Entity : public Core::IntrusiveNode<Entity> {
 protected:
  Device& device_;  // Non-const to allow registration
  std::string_view domain_;
  std::string_view object_id_;
  std::string_view name_;

  std::string discovery_topic_;
  std::string state_topic_;
  std::string command_topic_;

 public:
  constexpr Entity(Device& device, std::string_view domain, std::string_view object_id,
                   std::string_view name)
      : device_(device), domain_(domain), object_id_(object_id), name_(name) {
    device_.register_entity(this);
  }

  virtual ~Entity() = default;

  const std::string& get_discovery_topic() const { return discovery_topic_; }
  const std::string& get_state_topic() const { return state_topic_; }
  const std::string& get_command_topic() const { return command_topic_; }

  virtual void initialize_topics() = 0;
  virtual std::string get_discovery_payload() const = 0;
  virtual std::string get_state_payload() const = 0;

  // Default empty implementation. Sensors ignore this; Lights override it.
  virtual void handle_command(std::string_view /*payload*/) {}

 protected:
  void initialize_base_topics(bool expects_commands = false);
  // Bootstraps the standard JSON fields required by all HA entities
  void inject_base_config(JsonObjectBuilder& builder) const;
};

}  // namespace HAPPY