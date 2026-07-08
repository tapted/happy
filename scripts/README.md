# ESP32 Local OTA Development Workflow (ota_push.py)

This project utilizes a seamless, zero-touch Over-The-Air (OTA) update workflow for local network
development.

Instead of plugging the ESP32 into USB every time you want to test a code change, you can push
updates directly over your LAN using a single command: `idf.py ota`.

## 🧠 Context: The Architecture

Managing firmware versions during rapid local development presents a classic "Chicken and Egg"
problem:

1. If the ESP32 doesn't see a *newer* version number in the update manifest, it will refuse to
   download the binary.
2. If we hardcode the version in C++ or CMake, we have to manually bump it every single build, or
   the ESP32 will ignore the update.
3. If we use a Git Hash for the version, the version only changes when we make a commit—which we
   don't want to do for every minor typo fix during local debugging.

**The Solution:** We decouple the firmware version from the C++ compiler. When you run `idf.py ota`,
our custom build chain compiles the binary, and then passes control to `ota_push.py`. This script
tracks your current Git Hash and maintains a local incrementing revision counter (e.g.,
`1.0.0-a1b2c3d-r4`). It generates a `manifest.json` with this new dynamic version, starts a
temporary HTTP server, and uses MQTT to "poke" the ESP32.

The ESP32 blindly trusts the version string in the manifest. Once it successfully downloads and
flashes the new binary, it saves this new version string to its internal NVS (Non-Volatile Storage)
so it knows its own identity on the next boot.

---

## 🚀 Setup & Installation

### 1. Prerequisites

You need the Mosquitto MQTT Python client installed on your development machine:

```bash
pip install paho-mqtt

```

### 2. The Python Script

Ensure the OTA script is located at `deps/happy/scripts/ota_push.py` in your project root. *(If you
haven't created this file yet, it handles dynamic versioning, hosts a python `http.server`, and
publishes the MQTT trigger).*

### 3. Git Configuration

Because the script maintains a local revision counter, you must prevent this state file from being
committed to your repository. Add the following to your `.gitignore`:

```text
# Ignore local OTA revision tracker
.ota_revision.json

```

### 4. CMake Integration

To bind the Python script to the ESP-IDF build system, add this custom target to the **bottom** of
your root `CMakeLists.txt` (the one in the root directory, not the one inside the `main/` folder).

```cmake
# --- Local Network OTA Target ---

# Fallback base version if not defined earlier in CMake
if(NOT PROJECT_VER)
    set(PROJECT_VER "1.0.0")
endif()

# Define the custom 'ota' command
add_custom_target(ota
    # 1. Ensure the C++ firmware is built first
    DEPENDS app 
    
    # 2. Execute the python script
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/ota_push.py
        --bin ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
        --project ${CMAKE_PROJECT_NAME}
        --base-version ${PROJECT_VER}
        --broker "10.1.0.201"             # <--- UPDATE THIS to your MQTT broker IP
        --topic "puck_v1/ota_trigger/set" # <--- UPDATE THIS to your device's trigger topic
        
    # Run the script from the root directory so it can access .git
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    
    COMMENT "Building and pushing OTA update to ESP32..."
    USES_TERMINAL
)

```

---

## 🛠️ Usage

Whenever you want to push your code to the ESP32 over Wi-Fi, simply run:

```bash
idf.py ota

```

### What happens under the hood?

1. **Compilation:** `ninja` checks for C++ changes and rebuilds your `.bin` file.
2. **Version Generation:** `ota_push.py` checks your current Git commit hash. If the hash hasn't
   changed since the last OTA, it increments the `-r#` counter (e.g., `r1` -> `r2`). If the hash has
   changed, it resets the counter to `r1`.
3. **Manifest Creation:** It writes a `manifest.json` into the `build/` directory containing the
   dynamic version and the binary filename.
4. **Hosting:** It spins up a lightweight HTTP server on port `8032`.
5. **Trigger:** It publishes the string `"PRESS"` to your specified MQTT topic.
6. **Download:** The ESP32 receives the MQTT message, fetches the manifest, sees the newer version,
   and downloads the `.bin` file.
7. **Cleanup:** The Python script detects the `.bin` file being requested by the ESP32. It logs a
   success message and immediately shuts down the HTTP server, returning control to your terminal.

---

## ⚠️ Firmware Requirements (C++ Side)

For this workflow to succeed, your ESP32 firmware must be configured to handle the update securely:

1. **`menuconfig` Settings:**
* You must enable HTTP for OTA (by default, ESP-IDF requires HTTPS/TLS). `Component config -> ESP
HTTPS OTA -> Allow HTTP for OTA`
* You should enable App Rollback to prevent bricking the device if your new code crashes.
`Bootloader config -> App Rollback Support`


2. **NVS Versioning:**
* Your C++ OTA controller must save the newly downloaded version string into NVS upon a successful
  `esp_https_ota()` flash, just before calling `esp_restart()`.


3. **Rollback Confirmation:**
* Because App Rollback is enabled, the new firmware boots in an "unconfirmed" state. Once your
  device successfully connects to Wi-Fi and MQTT in `app_main`, you **must** call
  `esp_ota_mark_app_valid_cancel_rollback()`. If you fail to call this, the ESP32 will assume the
  update is broken and revert to the old firmware on the next reboot.



## 🐛 Troubleshooting

* **Script times out waiting for download:** Ensure your development machine's firewall is not
  blocking incoming TCP connections on port `8032`. The ESP32 must be able to route to your
  machine's local IP.
* **Update loops infinitely:** The ESP32 is failing to save the new version string to NVS before
  rebooting. It wakes up, thinks it is still running the old version, and downloads the update again
  the next time the button is pressed.
* **Firmware reverts to old version after successful OTA:** Your new C++ code is crashing before it
  can call `esp_ota_mark_app_valid_cancel_rollback()`, or you forgot to include that function call
  in your boot sequence. The hardware watchdog is doing its job and safely rolling back!