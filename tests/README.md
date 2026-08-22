# HAPPY Integration Test Harness

This directory contains standalone integration regression tests for the `HAPPY` library (`deps/happy`). It tests entities (such as `Sensor`, `Light`, `Button`, `Alarm`, `SystemDiagnostics`) against `MqttDevice` and a mock MQTT library.

## Architecture & Design

1. **Standalone & Self-Contained**: Uses CMake `FetchContent` to download GoogleTest (v1.17.0) automatically.
2. **Mock MQTT Transport (`mock_mqtt`)**: Emulates the ESP-IDF MQTT client (`mqtt_client.h`). Records all calls to `esp_mqtt_client_enqueue`, `esp_mqtt_client_subscribe_single`, and `esp_mqtt_client_publish`, and simulates MQTT events (like `MQTT_EVENT_CONNECTED`, `MQTT_EVENT_PUBLISHED`, `MQTT_EVENT_SUBSCRIBED`).
3. **Integration Test Harness (`HappyIntegrationTestHarness`)**: All entity test suites inherit from `HappyIntegrationTestHarness`. It manages an `MqttDevice` instance, hooks up the mock MQTT client, drives the pumping queue until idle (`connect_and_pump_until_idle()`), and provides helper methods to inspect MQTT calls and payloads.

---

## Build & Run Instructions

### Command Line

```powershell
cmake -B deps/happy/tests/build -S deps/happy/tests
cmake --build deps/happy/tests/build
deps/happy/tests/build/happy_tests.exe
```

---

## VS Code Setup

To build and run these tests inside Visual Studio Code using CMake Tools alongside other projects, update `.vscode/settings.json`:

```json
{
  "cmake.sourceDirectory": [
    "${workspaceFolder}/sdlmain",
    "${workspaceFolder}/deps/espbase/tests",
    "${workspaceFolder}/deps/happy/tests"
  ]
}
```

Then open the VS Code Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`) and choose:
**`CMake: Select Active Folder`** -> **`deps/happy/tests`**.
