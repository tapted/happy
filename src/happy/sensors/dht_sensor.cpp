#include "happy/sensors/dht_sensor.hpp"

#if __has_include(<dht.h>)
#include <dht.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace HAPPY::Sensors {

bool DhtSensorReader::update() {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // The DHT sensors require ~2 seconds of hardware recovery time between reads.
  // If we are polled twice in quick succession, skip the hardware read entirely!
  if (has_read_ && (now - last_read_ms_ < 5000)) {
    return false;
  }

  EspResult<DHTReading> res = read();
  if (res) {
    temp_tenths_ = res->temperature_tenths;
    hum_tenths_ = res->humidity_tenths;
    last_read_ms_ = now;
    has_read_ = true;
    return true;
  }

  // DHT driver handles checksum failures and physical timeouts internally
  res.strip().log_error("DhtSensorReader", "Failed to read DHT sensor");
  return false;
}

EspResult<DHTReading> DhtSensorReader::read() {
  DHTReading reading = {};
  dht_sensor_type_t type;
  switch (type_) {
    case DHTType::DHT11:
      type = DHT_TYPE_DHT11;
      break;
    case DHTType::AM2301:
      type = DHT_TYPE_AM2301;
      break;
    case DHTType::SI7021:
      type = DHT_TYPE_SI7021;
      break;
    default:
      return ESP_ERR_INVALID_ARG;
  }
  if (EspError err =
          dht_read_data(type, pin_, &reading.humidity_tenths, &reading.temperature_tenths)) {
    return err;
  }
  return reading;
}

}  // namespace HAPPY::Sensors

#else
#pragma message("Install esp-idf-lib/dht to use DHT sensors")
#endif