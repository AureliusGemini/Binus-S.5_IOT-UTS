#include "WifiHandler.h"
#include <WiFi.h>

// WiFiManager is only included if we are NOT in the Wokwi simulation
#ifndef WOKWI_SIM
#include <WiFiManager.h>
#endif

void setup_wifi()
{
    Serial.print("[WiFi] Connecting...");

#ifdef WOKWI_SIM
    // --- WOKWI SIMULATION CODE ---
    Serial.println(" (Wokwi Mode)");
    WiFi.begin("Wokwi-GUEST", "");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
#else
    // --- REAL HARDWARE (WiFiManager) CODE ---
    Serial.println(" (Hardware Mode)");
    WiFiManager wm;

    // Fetches SSID and PWD from EEPROM
    // or starts an AP "AutoConnectAP" if not found
    if (!wm.autoConnect("ESP32_IOT_Exam"))
    {
        Serial.println("[WiFi] Failed to connect and hit timeout. Restarting...");
        ESP.restart();
    }
#endif

    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
}