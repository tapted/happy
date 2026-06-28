#pragma once

#include <functional>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class Time : public Entity {
 public:
  struct Config {
    const char* icon = "mdi:alarm";
    std::function<void(const Time&)> on_update = nullptr;
  };

  Time(Device& device, std::string_view object_id, std::string_view name, Config config)
      : Entity(device, "time", object_id, name), config_(std::move(config)) {}

  uint8_t hour() const { return hour_; }
  uint8_t minute() const { return minute_; }
  uint8_t second() const { return second_; }

  void initialize_topics() override { initialize_base_topics(true); }
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;
  void handle_command(std::string_view payload) override;

 private:
  Config config_;
  uint8_t hour_ = 7;
  uint8_t minute_ = 0;
  uint8_t second_ = 0;
};

}  // namespace HAPPY::Entities