# HAPPY (Home Assistant Plus Plus - Your-way)

HAPPY is a modern C++ library for ESP-IDF that provides a modular, protocol-agnostic framework for building smart home entities. 

Unlike opinionated frameworks that dictate your build system, hardware lifecycle, or configuration files (YAML), HAPPY is designed for embedded engineers who want complete control over their FreeRTOS tasks and memory allocations, while still seamlessly integrating with smart home ecosystems like Home Assistant.

## The Manifesto: Hardware-First, Protocol-Second

A light bulb, a temperature sensor, and an alarm clock are physical realities. The way they communicate their state to a network is an implementation detail. 

Most existing libraries tightly couple the entity's state definition directly to a specific network protocol (e.g., hardcoding MQTT topic strings inside a Light class, or tying sensor polling strictly to a Matter cluster). 

HAPPY separates the **Entity** from the **Transport**. 

You define your hardware logic and state transitions once. You then pass that entity to a Transport implementation (like MQTT Auto-Discovery, or eventually, Matter). The transport layer handles the protocol-specific translations, while your application code remains entirely focused on hardware actuation.

## Design Philosophy

* **You Own `app_main()`:** HAPPY does not take over your main loop. It exposes standard C++ objects that you step manually, allowing you to prioritize hardware interrupts, custom UI rendering (e.g., LVGL), or custom FreeRTOS task scheduling.
* **Zero-Allocation Hot Paths:** Built for modern C++26. Network payloads, state parsing, and JSON generation are designed to run without triggering heap fragmentation during the main runtime loop.
* **Modular Transports:** Entities are completely decoupled from network protocols. Write a `Light` entity once, and expose it via MQTT today, or Matter tomorrow.
* **RAII & Type Safety:** Hardware resources and MQTT subscriptions are managed strictly through standard object lifetimes. No floating global state.

## Architecture

HAPPY is split into two primary domains:

1. **Entities (`HAPPY::Entity`)**: Stateful representations of physical devices (Lights, Sensors, Switches). They maintain their own state and expose clean C++ callbacks for hardware actuation.
2. **Transports (`HAPPY::Transport`)**: The networking layer. A transport acts as a registry for entities, translating their state into protocol-specific data (e.g., JSON payloads over TCP for MQTT, or Endpoint Clusters for Matter).

### Current Focus: MQTT Auto-Discovery
The current stable transport implements Home Assistant MQTT Auto-Discovery. It natively handles device grouping, topic registration, and zero-copy JSON parsing using `cJSON`.

## Quick Start (MQTT Example)

```cpp
#include "happy/entities/light.hpp"
#include "happy/transports/mqtt_device.hpp"
#include "halpp/led_strip/led_strip.hpp"

// 1. Define the Transport Registry (The physical device footprint)
HAPPY::Transports::MqttDevice puck_device({
  .identifiers = "puck_v1_001",
  .name = "Bedroom Puck",
  .manufacturer = "Custom C++ Firmware"
});

// 2. Define the Entity, completely decoupled from network logic
HAPPY::Entities::Light onboard_led(puck_device, "status_led", "Onboard LED", {
  .supports_rgb = true,
  .on_update = [](const HAPPY::Entities::Light& light) {
    auto& strip = HAL::LedStrip::default_instance();
    auto [r, g, b] = light.scaled_rgb();
    
    strip.set_pixel(0, r, g, b);
    strip.refresh();
  }
});

void app_main() {
  // Initialize your MQTT client and route incoming data to the registry.
  // puck_device.dispatch_command(topic, payload); handles the rest.
}