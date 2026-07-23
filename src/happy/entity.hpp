#pragma once

#include <atomic>
#include <cstring>
#include <string>
#include <string_view>

#include "espbase/json_fwd.h"
#include "happy/core/intrusive_list.hpp"

namespace HAPPY {
class Device;

template <size_t N>
std::string buf2str(const char (&buf)[N]) {
  return std::string(buf, strnlen(buf, N));
}

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
  static constexpr uint8_t FLAG_DISCOVERY = 1 << 0;
  static constexpr uint8_t FLAG_SUBSCRIBE = 1 << 1;
  static constexpr uint8_t FLAG_STATE = 1 << 2;
  static constexpr uint8_t EPHEMERAL_STATE_QOS = 1 << 3;
  static constexpr uint8_t RETAIN_STATE = 1 << 4;

  Entity(Device& device, std::string_view domain, std::string_view object_id,
         std::string_view name);

  void request_publish();
  void request_discovery();

  uint8_t get_pending_flags() const { return pending_flags_.load(std::memory_order_acquire); }
  int get_state_qos() const { return (get_pending_flags() & EPHEMERAL_STATE_QOS) ? 0 : 1; }
  int get_state_retain() const { return (get_pending_flags() & RETAIN_STATE) ? 1 : 0; }
  void clear_flag(uint8_t flag) { pending_flags_.fetch_and(~flag, std::memory_order_release); }

  virtual ~Entity() = default;

  const std::string& get_discovery_topic() const { return discovery_topic_; }
  const std::string& get_state_topic() const { return state_topic_; }
  const std::string& get_command_topic() const { return command_topic_; }

  virtual void load() {}
  virtual void initialize_topics() = 0;
  virtual std::string get_discovery_payload() const = 0;
  virtual std::string get_state_payload() const { return std::string(); }

  // Default empty implementation. Sensors ignore this; Lights override it.
  virtual void handle_command(std::string_view /*payload*/) {}

 protected:
  std::atomic<uint8_t> pending_flags_{0};

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
 public:
  const StateStruct& state() const { return state_; }

 protected:
  using Entity::Entity;

  void load() override {
    static_assert(std::is_trivially_copyable_v<StateStruct>,
                  "PersistentEntity StateStruct must be a trivially copyable POD type.");
    if (this->load_nvs_blob(&state_, sizeof(StateStruct))) {
      this->on_change();
    }
  }
  virtual void on_change() = 0;

  void set_state(StateStruct new_state) {
    if (std::memcmp(&state_, &new_state, sizeof(StateStruct)) == 0) {
      return;  // No change in state, do nothing
    }
    state_ = new_state;
    this->save_nvs_blob(&state_, sizeof(StateStruct));
    this->request_publish();
    this->on_change();
  }

 private:
  StateStruct state_{};
};

}  // namespace HAPPY