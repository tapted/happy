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

  void publish() const { device_.publish(*this); }

  virtual ~Entity() = default;

  const std::string& get_discovery_topic() const { return discovery_topic_; }
  const std::string& get_state_topic() const { return state_topic_; }
  const std::string& get_command_topic() const { return command_topic_; }

  virtual void initialize_topics() = 0;
  virtual std::string get_discovery_payload() const = 0;
  virtual std::string get_state_payload() const { return std::string(); }

  // Default empty implementation. Sensors ignore this; Lights override it.
  virtual void handle_command(std::string_view /*payload*/) {}

 protected:
  void initialize_base_topics(bool expects_commands = false);
  bool load_nvs_blob(void* dest, size_t size) const;
  void save_nvs_blob(const void* src, size_t size) const;

  // Bootstraps the standard JSON fields required by all HA entities
  void inject_base_config(JsonObjectBuilder& builder) const;
};

// PersistentEntity is a template class that extends Entity to provide automatic state persistence
// using NVS (Non-Volatile Storage). It requires the derived class and a trivially copyable state
// struct as template parameters. The state is automatically loaded from NVS during initialization
// and can be saved back to NVS when needed.
template <typename Derived, typename StateStruct>
class PersistentEntity : public Entity {
 protected:
  StateStruct state_{};

  using Entity::Entity;

  // Returns true if the state was successfully loaded from NVS, false otherwise.
  bool initialize_base_topics(bool expects_commands = false) {
    Entity::initialize_base_topics(expects_commands);
    static_assert(std::is_trivially_copyable_v<StateStruct>,
                  "PersistentEntity StateStruct must be a trivially copyable POD type.");
    return this->load_nvs_blob(&state_, sizeof(StateStruct));
  }

  void save_state() const { this->save_nvs_blob(&state_, sizeof(StateStruct)); }
};

}  // namespace HAPPY