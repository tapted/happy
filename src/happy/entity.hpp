#pragma once

#include <atomic>
#include <cstring>
#include <string_view>

#include "happy/core/intrusive_list.hpp"

namespace sjson {
class Buffer;
class Builder;
class Printer;
}  // namespace sjson

namespace HAPPY {
class Device;

using topic_buf_t = char[128];

class Entity : public Core::IntrusiveNode<Entity> {
 protected:
  Device& device_;  // Non-const to allow registration
  const char* const domain_;
  const char* const object_id_;
  const char* const name_;

 public:
  static constexpr uint8_t FLAG_DISCOVERY = 1 << 0;
  static constexpr uint8_t FLAG_SUBSCRIBE = 1 << 1;
  static constexpr uint8_t FLAG_STATE = 1 << 2;
  static constexpr uint8_t PENDING_MASK = FLAG_DISCOVERY | FLAG_SUBSCRIBE | FLAG_STATE;
  static constexpr uint8_t EPHEMERAL_STATE_QOS = 1 << 3;
  static constexpr uint8_t RETAIN_STATE = 1 << 4;
  static constexpr uint8_t EXPECTS_COMMANDS = 1 << 5;

  Entity(Device& device, const char* domain, const char* object_id, const char* name,
         bool expects_commands);

  void request_publish();
  void request_discovery();

  uint8_t get_pending_flags() const { return get_flags() & PENDING_MASK; }
  int get_state_qos() const { return (get_flags() & EPHEMERAL_STATE_QOS) ? 0 : 1; }
  int get_state_retain() const { return (get_flags() & RETAIN_STATE) ? 1 : 0; }
  bool expects_commands() const { return (get_flags() & EXPECTS_COMMANDS) != 0; }

  void clear_flag(uint8_t flag) { flags_.fetch_and(~flag, std::memory_order_release); }
  void set_flag(uint8_t flag) { flags_.fetch_or(flag, std::memory_order_release); }

  virtual ~Entity() = default;

  void get_discovery_topic(topic_buf_t& buf) const;

  void print_state_topic(sjson::Printer& print) const;
  const char* get_state_topic(topic_buf_t& buf) const;
  void get_command_topic(topic_buf_t& buf) const;

  virtual void load() {}
  virtual bool get_discovery_payload(sjson::Buffer& buffer) = 0;
  virtual size_t get_state_payload(sjson::Buffer& buffer);

  // Default empty implementation. Sensors ignore this; Lights override it.
  virtual void handle_command(std::string_view /*payload*/) {}

 protected:
  std::atomic<uint8_t> flags_{0};

  uint8_t get_flags() const { return flags_.load(std::memory_order_acquire); }

  bool load_nvs_blob(void* dest, size_t size) const;
  void save_nvs_blob(const void* src, size_t size) const;

  // Bootstraps the standard JSON fields required by all HA entities
  bool emit_with_base_config(sjson::Buffer& buffer, sjson::Builder& builder) const;
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