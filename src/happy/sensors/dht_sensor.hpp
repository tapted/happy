#pragma once

#include <driver/gpio.h>

#include "espbase/esp_result.hpp"
#include "happy/entities/lazy_sensor.hpp"

// idf_component.yml
//   esp-idf-lib/dht: '^1.2.*'
//
// #include "happy/entities//lazy_sensor.hpp"
// #include "happy/sensors/dht_sensor.hpp"
//
// static constinit HAPPY::Sensors::DhtSensorReader dht_reader(GPIO_NUM_4,
//                                                             HAPPY::Sensors::DHTType::DHT11);
// static constinit HAPPY::Entities::SensorState<int16_t> temp_state(
//     []() -> void { dht_reader.update(); }, []() -> int16_t { return dht_reader.get_temp(); },
//     HAPPY::Entities::format_tenths);
// static constinit HAPPY::Entities::SensorState<int16_t> hum_state(
//     []() -> void { dht_reader.update(); }, []() -> int16_t { return dht_reader.get_humidity(); },
//     HAPPY::Entities::format_tenths);
// static HAPPY::Entities::LazySensor temp_entity(my_device, "dht11_temp", "DHT11 Temperature",
//                                                {
//                                                    .device_class = "temperature",
//                                                    .unit_of_measurement = "°C",
//                                                },
//                                                &temp_state);
// static HAPPY::Entities::LazySensor hum_entity(my_device, "dht11_hum", "DHT11 Humidity",
//                                               {
//                                                   .device_class = "humidity",
//                                                   .unit_of_measurement = "%",
//                                               },
//                                               &hum_state);

namespace HAPPY::Sensors {

enum class DHTType {
  DHT11,
  AM2301,  // (DHT21, DHT22, AM2302, AM2321)
  SI7021
};

struct DHTReading {
  int16_t temperature_tenths;
  int16_t humidity_tenths;
};

class DhtSensorReader : public HAPPY::Entities::SensorReader {
 public:
  esp_err_t last_error = ESP_OK;
  constexpr DhtSensorReader(gpio_num_t pin = GPIO_NUM_4, DHTType type = DHTType::DHT11)
      : pin_(pin), type_(type) {}

  bool refresh() override;

  int16_t get_temp() const { return temp_tenths_; }
  int16_t get_humidity() const { return hum_tenths_; }

 private:
  EspResult<DHTReading> read();

  gpio_num_t pin_;
  DHTType type_;

  uint32_t last_read_ms_ = 0;
  int16_t temp_tenths_ = 0;
  int16_t hum_tenths_ = 0;
};

}  // namespace HAPPY::Sensors