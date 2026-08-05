#pragma once

#include <functional>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct TimeState {
  uint8_t hour_ = 7;
  uint8_t minute_ = 0;
  uint8_t second_ = 0;
};

class Time : public PersistentEntity<Time, TimeState> {
 public:
  struct Config {
    const char* icon = "mdi:alarm";
    void (*on_update)(void*, const Time&) = nullptr;
  };

  Time(Device& device, const char* object_id, const char* name, Config config,
       void* on_update_ctx = nullptr)
      : PersistentEntity(device, "time", object_id, name),
        config_(std::move(config)),
        on_update_ctx(on_update_ctx) {}

  uint8_t hour() const { return state().hour_; }
  uint8_t minute() const { return state().minute_; }
  uint8_t second() const { return state().second_; }

  std::string get_discovery_payload() override;
  std::string get_state_payload() override;
  void handle_command(std::string_view payload) override;
  void on_change() override {
    if (config_.on_update) config_.on_update(on_update_ctx, *this);
  }

 private:
  Config config_;
  void* on_update_ctx = nullptr;
};

}  // namespace HAPPY::Entities