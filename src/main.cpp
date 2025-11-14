#include <Arduino.h>
#include "config.h"        // Global settings (MQTT, Timings)
#include "WifiHandler.h"   // Wi-Fi connection logic
#include "SensorManager.h" // Sensor setup
#include "TaskManager.h"   // FreeRTOS task setup

/**
 * @brief Main setup function, runs once on boot.
 */
void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  } // Wait for serial

  Serial.println("\n[Main] Booting up...");

  // 1. Connect to Wi-Fi (handles Wokwi vs. WifiManager)
  setup_wifi();

  // 2. Initialize sensors
  // I2C pins are now passed to setup_sensors() from build flags
  setup_sensors();

  // 3. Create and start all FreeRTOS tasks
  // This function will create the MQTT and LED tasks.
  setup_rtos_tasks();

  Serial.println("[Main] Setup complete. Handing over to RTOS.");
}

/**
 * @brief Main loop.
 * With FreeRTOS, this loop is idle. It runs on Core 1 as the
 * 'loopTask' with priority 1 (low). We can just let it sleep.
 */
void loop()
{
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}