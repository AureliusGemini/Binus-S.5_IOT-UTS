#include "TaskManager.h"
#include "config.h"
#include "SensorManager.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- RTOS & MQTT Objects ---
static SemaphoreHandle_t alertSemaphore; // For signaling from MQTT to LED task
static WiFiClient espClient;
static PubSubClient client(espClient);

// --- Local Function Prototypes ---
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void reconnect_mqtt();

/**
 * @brief FreeRTOS Task 1: Handles MQTT, Wi-Fi, and publishing sensor data.
 * (Fulfills Part I)
 */
void mqttTask(void *pvParameters)
{
    Serial.println("[MQTT Task] Starting...");
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqtt_callback);

    TickType_t lastPublishTime = xTaskGetTickCount();

    for (;;)
    {
        // 1. Maintain MQTT connection
        if (!client.connected())
        {
            reconnect_mqtt();
        }
        client.loop(); // Handles subscriptions and internal messages

        // 2. Publish sensor data every PUBLISH_INTERVAL_MS
        // (Fulfills Part I)
        if (xTaskGetTickCount() - lastPublishTime > (PUBLISH_INTERVAL_MS / portTICK_PERIOD_MS))
        {
            lastPublishTime = xTaskGetTickCount();

            float t, h, l;
            if (read_all_sensors(t, h, l))
            {
                // Determine if lux is saturated at the upper clamp
                bool luxSaturated = (l >= 65535.0f - 0.01f);

                char payload[220];
                if (luxSaturated)
                {
                    // When saturated, override the lux field with a string value
                    snprintf(payload, sizeof(payload),
                             "{\"student\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f, \"lux\": \"SATURATED\"}",
                             STUDENT_NAME, t, h);
                }
                else
                {
                    // Normal numeric lux
                    snprintf(payload, sizeof(payload),
                             "{\"student\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f, \"lux\": %.2f}",
                             STUDENT_NAME, t, h, l);
                }

                Serial.print("[MQTT Task] Publishing data: ");
                Serial.println(payload);

                client.publish(MQTT_TOPIC_PUB, payload);
            }
        }

        // Yield to other tasks for a moment
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief FreeRTOS Task 2: Manages the onboard LED.
 * (Fulfills Part II)
 */
void ledTask(void *pvParameters)
{
    Serial.println("[LED Task] Starting...");
    pinMode(PIN_LED, OUTPUT);

    for (;;)
    {
        // 3. Wait for an alert signal (with a 10ms timeout)
        // (Fulfills Part II)
        if (xSemaphoreTake(alertSemaphore, (TickType_t)10) == pdTRUE)
        {
            // --- ALERT STATE ---
            // We got the signal! Turn LED on solid for 10 seconds.
            Serial.println("[LED Task] ALERT received! LED ON SOLID.");
            digitalWrite(PIN_LED, HIGH);
            vTaskDelay(ALERT_DURATION_MS / portTICK_PERIOD_MS);
            Serial.println("[LED Task] Alert finished. Resuming blink.");
        }
        else
        {
            // --- NORMAL BLINK STATE ---
            // No alert, just do the low-priority blink.
            digitalWrite(PIN_LED, HIGH);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            digitalWrite(PIN_LED, LOW);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

/**
 * @brief Called by PubSubClient when a message arrives.
 */
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("[MQTT Callback] Message arrived on topic: ");
    Serial.println(topic);

    // Convert payload to a string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.print("[MQTT Callback] Payload: ");
    Serial.println(message);

    // Check if it's the alert message
    if (strcmp(message, MQTT_PAYLOAD_ALERT) == 0)
    {
        // This is the core of Part II:
        // Give the semaphore to signal the ledTask.
        // This is thread-safe and interrupt-safe.
        Serial.println("[MQTT Callback] ALERT detected! Giving semaphore.");
        xSemaphoreGive(alertSemaphore);
    }
}

/**
 * @brief Connects (or reconnects) to the MQTT broker. *
 **/
void reconnect_mqtt()
{
    while (!client.connected())
    {
        Serial.print("[MQTT Task] Attempting connection...");
        String clientId = "esp32-" + String(STUDENT_ID);

        if (client.connect(clientId.c_str()))
        {
            Serial.println(" connected!");
            // Subscribe to the control topic
            client.subscribe(MQTT_TOPIC_SUB);
            Serial.print("[MQTT Task] Subscribed to: ");
            Serial.println(MQTT_TOPIC_SUB);
        }
        else
        {
            Serial.print(" failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait before retrying
        }
    }
}

/**
 * @brief Main setup function for this file.
 */
void setup_rtos_tasks()
{
    Serial.println("[RTOS] Creating tasks...");

    // Create the binary semaphore.
    alertSemaphore = xSemaphoreCreateBinary();

    // Create Task 1: MQTT Handler (Higher Priority)
    // Runs on Core 0 or 1 (determined by scheduler)
    xTaskCreate(
        mqttTask,
        "MQTT Task",
        4096,  // Stack size (bytes)
        NULL,  // Task parameters
        2,     // Priority (higher than LED task)
        NULL); // Task handle (optional)

    // Create Task 2: LED Blinker (Lower Priority)
    // Runs on Core 0 or 1 (determined by scheduler)
    xTaskCreate(
        ledTask,
        "LED Task",
        1024,  // Stack size (bytes)
        NULL,  // Task parameters
        1,     // Priority (lower than MQTT task)
        NULL); // Task handle (optional)

    Serial.println("[RTOS] Tasks created.");
}