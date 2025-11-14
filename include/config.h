#pragma once

// --- STUDENT INFO ---
#define STUDENT_NAME "Elvin Aurelius Yamin"
#define STUDENT_ID "2702407514"

// --- MQTT SETTINGS ---
// These are common to all environments
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TOPIC_PUB STUDENT_ID "/environment"
#define MQTT_TOPIC_SUB STUDENT_ID "/control/LED"
#define MQTT_PAYLOAD_ALERT "ALERT"

// --- RTOS & TIMING SETTINGS ---
#define PUBLISH_INTERVAL_MS 5000
#define ALERT_DURATION_MS 10000

// --- HARDWARE PINS ---
// All hardware pins (PIN_LED, PIN_DHT, PIN_SDA, PIN_SCL)
// are now defined in platformio.ini under 'build_flags'
// for each specific environment.