#pragma once

#include <cmath>
#include <string>
#include <type_traits>

#include "espbase/main_loop.hpp"
#include "happy/entities/sensor.hpp"

namespace HAPPY::Entities {

// A reusable, allocation-free formatter for tenths-of-a-degree integers
inline std::string format_tenths(const int16_t& val) {
  char buf[16];
  int abs_val = std::abs(val);
  snprintf(buf, sizeof(buf), "%s%d.%d", (val < 0) ? "-" : "", abs_val / 10, abs_val % 10);
  return std::string(buf);
}

class AbstractSensorState {
 public:
  virtual ~AbstractSensorState() = default;

  // Triggers the underlying hardware reader to fetch new data (if necessary)
  virtual void refresh() = 0;

  // Evaluates the current cached value against the last published value
  virtual bool needs_publish() = 0;

  // Returns the string payload and internally updates the last_published_value marker
  virtual std::string get_payload() = 0;
};

template <typename T>
class SensorState : public AbstractSensorState {
 public:
  using refresh_t = bool (*)();
  using fetch_t = T (*)();
  using formatter_t = std::string (*)(const T&);

  constexpr SensorState(refresh_t refresh_cb, fetch_t fetch_cb, formatter_t formatter = nullptr)
      : refresh_cb_(refresh_cb), fetch_cb_(fetch_cb), formatter_(formatter) {
    needs_successful_refresh_ = refresh_cb_ != nullptr;
  }

  void refresh() override {
    if (!refresh_cb_) return;

    bool success = refresh_cb_();
    if (success && needs_successful_refresh_) {
      needs_successful_refresh_ = false;
    }
  }

  bool needs_publish() override {
    if (needs_successful_refresh_) return false;

    T current_val = fetch_cb_();

    if constexpr (std::is_floating_point_v<T>) {
      // Small epsilon check to prevent floating-point analog jitter from spamming the network
      return std::abs(current_val - last_published_value_) > 0.05f;
    } else {
      return current_val != last_published_value_;
    }
  }

  std::string get_payload() override {
    if (needs_successful_refresh_) return "unknown";

    T current_val = fetch_cb_();
    last_published_value_ = current_val;

    if (formatter_) {
      return formatter_(current_val);
    }

    if constexpr (std::is_floating_point_v<T>) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%.1f", current_val);
      return std::string(buf);
    } else {
      return std::to_string(current_val);
    }
  }

 private:
  refresh_t refresh_cb_;
  fetch_t fetch_cb_;
  formatter_t formatter_;

  T last_published_value_{};
  bool needs_successful_refresh_ = false;
};

class LazySensor : public Sensor {
 public:
  LazySensor(Device& device, const char* object_id, const char* name, Config config,
             AbstractSensorState* state)
      : Sensor(device, object_id, name, patch_config(std::move(config)), state), state_(state) {}

  void publish_if_changed() override {
    // Escapes the current calling context and pushes the blocking hardware reads to the MainLoop
    main_loop.push<&LazySensor::refresh_and_maybe_publish>(this);
  }

  void refresh_and_maybe_publish() {
    state_->refresh();
    if (state_->needs_publish()) {
      request_publish();  // Eventually calls get_payload() via the standard Entity pipeline
    }
  }

 private:
  AbstractSensorState* state_;

  static Config patch_config(Config config) {
    config.get_value = trampoline<&AbstractSensorState::get_payload>();
    return config;
  }
};

}  // namespace HAPPY::Entities