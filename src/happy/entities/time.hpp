#pragma once

#include <functional>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct TimeState {
  uint8_t hour_ = 7;
  uint8_t minute_ = 0;
  uint8_t second_ = 0;
};

class Time : public  PersistentEntity<Time, TimeState> {
 public:
  struct Config {
    const char* icon = "mdi:alarm";
    std::function<void(const Time&)> on_update = nullptr;
  };

  Time(Device& device, std::string_view object_id, std::string_view name, Config config)
      : PersistentEntity(device, "time", object_id, name), config_(std::move(config)) {}

  uint8_t hour() const { return state_.hour_; }
  uint8_t minute() const { return state_.minute_; }
  uint8_t second() const { return state_.second_; }

  void initialize_topics() override;
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;
  void handle_command(std::string_view payload) override;
  void on_change() override {
    if (config_.on_update) config_.on_update(*this);
  }

 private:
  Config config_;
};

}  // namespace HAPPY::Entities