#pragma once

#include <string>
#include <type_traits>

#include "espbase/main_loop.hpp"
#include "happy/entities/sensor.hpp"

namespace HAPPY::Entities {

// A reusable, allocation-free formatter for tenths-of-a-degree integers
std::string format_tenths(const int16_t& val);

// The core interface for hardware polling
class SensorReader {
 public:
  constexpr SensorReader() = default;
  virtual ~SensorReader() = default;

  // Returns true on a successful refresh.
  virtual bool refresh() = 0;
};

class Poller : public SensorReader {
 public:
  bool refresh() override { return true; }
};

class AbstractSensorState {
 public:
  inline static constinit Poller poll_reader{};

  explicit AbstractSensorState(SensorReader& reader) : reader_(reader) {}

  // Default constructor uses a stateless Poller that always returns true, and so there's no point
  // waiting for a sucessful refresh.
  AbstractSensorState() : reader_(poll_reader), needs_successful_refresh_(false) {}

  virtual ~AbstractSensorState() = default;

  // Triggers the underlying hardware reader to fetch new data (if necessary)
  void refresh() {
    bool success = reader_.refresh();
    if (success) needs_successful_refresh_ = false;  // Sticky after the first successful refresh.
  }

  // Evaluates the current cached value against the last published value
  bool needs_publish() {
    return !needs_successful_refresh_ && (!has_published_after_refresh_ || value_changed());
  }

  // Returns the string payload and internally updates the last_published_value marker
  std::string get_payload_for_publish() {
    has_published_after_refresh_ = has_published_after_refresh_ || !needs_successful_refresh_;
    return needs_successful_refresh_ ? "unknown" : get_payload_after_refresh();
  }

 protected:
  // Returns true if the cached value has changed since the last publish
  virtual bool value_changed() = 0;

  // Returns the string payload after a refresh() has been called. Updates the last_published_value
  virtual std::string get_payload_after_refresh() = 0;

  SensorReader& reader_;
  bool needs_successful_refresh_ = true;
  bool has_published_after_refresh_ = false;
};

template <typename T>
class SensorState : public AbstractSensorState {
 public:
  using fetch_t = T (*)();
  using formatter_t = std::string (*)(const T&);

  // Version that takes a SensorReader to coordinate with hardware that can't always return a
  // valid value, or uses internal caching to avoid excessive hardware reads.
  constexpr SensorState(SensorReader& reader, fetch_t fetch_cb, formatter_t formatter = nullptr)
      : AbstractSensorState(reader), fetch_cb_(fetch_cb), formatter_(formatter) {}

  // Version that uses a stateless reader and invokes the callback on each publish.
  constexpr SensorState(fetch_t fetch_cb, formatter_t formatter = nullptr)
      : fetch_cb_(fetch_cb), formatter_(formatter) {}

 protected:
  fetch_t fetch_cb_;
  formatter_t formatter_;
  T last_published_value_{};

  virtual T get_current_value() { return fetch_cb_(); }

  bool value_changed() override {
    T current_val = get_current_value();

    if constexpr (std::is_floating_point_v<T>) {
      // Small epsilon check to prevent floating-point analog jitter from spamming the network
      return std::abs(current_val - last_published_value_) > 0.05f;
    } else {
      return current_val != last_published_value_;
    }
  }

  std::string get_payload_after_refresh() override {
    T current_val = get_current_value();
    last_published_value_ = current_val;

    if (formatter_) return formatter_(current_val);

    if constexpr (std::is_same_v<T, std::string>) {
      return current_val;
    } else if constexpr (std::is_floating_point_v<T>) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%.4f", current_val);
      return std::string(buf);
    } else {
      return std::to_string(current_val);
    }
  }
};

template <typename T>
class CachingConstSensorState : public SensorState<T> {
 public:
  constexpr CachingConstSensorState(typename SensorState<T>::fetch_t fetch_cb,
                                    typename SensorState<T>::formatter_t formatter = nullptr)
      : SensorState<T>(fetch_cb, formatter) {}

  bool value_changed() override { return false; }

  T get_current_value() override {
    if (!fetched_) {
      this->last_published_value_ = this->fetch_cb_();
      fetched_ = true;
    }
    return this->last_published_value_;
  }

 private:
  bool fetched_ = false;
};

// Sensor that reads from a stateful container tracking the last published value.
class LazySensor : public Sensor {
 public:
  LazySensor(Device& device, const char* object_id, const char* name, Config config,
             AbstractSensorState* state)
      : Sensor(device, object_id, name, patch_config(std::move(config)), state), state_(state) {}

  // Posts to `main_loop` a refresh() on the underlying SensorState. Publishes if it changes.
  void publish_if_changed() { main_loop.push<&LazySensor::refresh_and_maybe_publish>(this); }

 private:
  AbstractSensorState* state_;

  void refresh_and_maybe_publish();

  static Config patch_config(Config config) {
    config.get_value = trampoline<&AbstractSensorState::get_payload_for_publish>();
    return config;
  }
};

// An all-in-one wrapper that owns its SensorState.
// @example
// static constinit Sensors::DhtSensorReader dht_reader(GPIO_NUM_16, Sensors::DHTType::DHT11);
// static Entities::StatefulSensor<int16_t> temp_entity(my_device, "temp", "Temperature",
//     { .device_class = "temperature", .unit_of_measurement = "°C" },
//     dht_reader, []() -> int16_t { return dht_reader.get_temp(); }, Entities::format_tenths);
template <typename T>
class StatefulSensor : public LazySensor {
 public:
  StatefulSensor(Device& device, const char* object_id, const char* name, Config config,
                 SensorReader& reader, typename SensorState<T>::fetch_t fetch_cb,
                 typename SensorState<T>::formatter_t formatter = nullptr)
      : LazySensor(device, object_id, name, std::move(config), &state_),
        state_(reader, fetch_cb, formatter) {}

 private:
  SensorState<T> state_;
};

template <typename T>
class PollingSensor : public LazySensor {
 public:
  PollingSensor(Device& device, const char* object_id, const char* name, Config config,
                typename SensorState<T>::fetch_t fetch_cb,
                typename SensorState<T>::formatter_t formatter = nullptr)
      : LazySensor(device, object_id, name, std::move(config), &state_),
        state_(fetch_cb, formatter) {}

 private:
  SensorState<T> state_;
};

template <typename T>
class CachingConstSensor : public LazySensor {
 public:
  CachingConstSensor(Device& device, const char* object_id, const char* name, Config config,
                     typename SensorState<T>::fetch_t fetch_cb,
                     typename SensorState<T>::formatter_t formatter = nullptr)
      : LazySensor(device, object_id, name, std::move(config), &state_),
        state_(fetch_cb, formatter) {}

 private:
  CachingConstSensorState<T> state_;
};

}  // namespace HAPPY::Entities