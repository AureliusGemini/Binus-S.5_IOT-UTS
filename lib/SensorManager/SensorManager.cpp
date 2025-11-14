#include "SensorManager.h"
// config.h is included via SensorManager.h or other files
#include <Wire.h>
#include <DHT.h>
#include <math.h>

// --- Conditional Includes ---
#ifdef WOKWI_SIM
// No extra include needed for LDR
#else
// Real hardware uses the BH1750
#include <BH1750.h>
#endif

// --- Sensor Objects ---

// Use DHT22 for Wokwi, DHT11 for real hardware (or just use DHT22 for both)
// The exam paper says DHT11, but your Wokwi part is a DHT22.
// We will tell the library to use the DHT22 part in the sim.
#ifdef WOKWI_SIM
DHT dht(PIN_DHT, DHT22);
#else
DHT dht(PIN_DHT, DHT11);
BH1750 lightMeter;
#endif

void setup_sensors()
{
    Serial.print("[Sensors] Initializing...");

#ifdef WOKWI_SIM
    // --- WOKWI SENSOR SETUP ---
    // The LDR is an analog sensor, set the pin as an input
    pinMode(PIN_LDR, INPUT);
    Serial.print(" LDR OK...");
#else
    // --- REAL HARDWARE SENSOR SETUP ---
    // Start I2C for BH1750
    Wire.begin(PIN_SDA, PIN_SCL);
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        Serial.print(" BH1750 OK...");
    }
    else
    {
        Serial.print(" BH1750 Error...");
    }
#endif

    // Start DHT sensor (common to both)
    dht.begin();
    Serial.print(" DHT OK...");
    Serial.println(" Done.");
}

bool read_all_sensors(float &t, float &h, float &l)
{
    h = dht.readHumidity();
    t = dht.readTemperature();

    // Check for failed DHT read
    if (isnan(h) || isnan(t))
    {
        Serial.println("[Sensors] Failed to read from DHT sensor!");
        return false;
    }

#ifdef WOKWI_SIM
    // --- WOKWI SENSOR READ ---
    // Read from the LDR (Photoresistor)
    // The 4-pin LDR module's AO pin is INVERTED.
    // Dark (slider left) = ~4095
    // Bright (slider right) = ~0
    // Instead of a naive linear map, approximate lux using a voltage-divider model
    // and an LDR gamma curve (rough fit for common GL55xx cells).
    //
    // Divider assumption (matches observed inversion):
    //   Vref --- R_DIV ---+--- AO --- LDR --- GND
    // So Vao = Vref * R_LDR / (R_DIV + R_LDR)
    // => R_LDR = R_DIV * Vao / (Vref - Vao)
    //
    // Then estimate lux using: R_LDR = RL10 * (10 / lux)^gamma
    // => lux = 10 * (RL10 / R_LDR)^(1/gamma)
    //
    // Constants below are approximate; tweak R_DIV, RL10, and GAMMA to match your module.
    int raw = analogRead(PIN_LDR);

#ifdef ARDUINO_ARCH_ESP32
    const int ADC_MAX = 4095; // ESP32 default 12-bit`
    const float VREF = 3.3f;  // Simulated board uses 3.3V
#else
    const int ADC_MAX = 1023; // Many AVR boards 10-bit
    const float VREF = 5.0f;  // Typical AVR reference
#endif

    const float R_DIV = 10000.0f; // 10k divider resistor (typical LDR module)
    const float RL10 = 50000.0f;  // LDR resistance at 10 lux (ohms), ~50k typical
    const float GAMMA = 0.7f;     // Slope for GL5528/GL5516 family (approx)

    // Convert ADC reading to voltage
    float v = (raw / (float)ADC_MAX) * VREF;

    // Compute LDR resistance; guard against divide-by-zero at extremes
    float rLdr;
    if (v <= 0.0005f)
    {
        rLdr = 1e6f; // extremely bright => very low V => cap at 1M to avoid inf
    }
    else if ((VREF - v) <= 0.0005f)
    {
        rLdr = 1e6f; // extremely dark => V ~ VREF => cap to avoid blow-up
    }
    else
    {
        rLdr = R_DIV * v / (VREF - v);
    }

    // Estimate lux via gamma model
    float lux = 10.0f * powf(RL10 / rLdr, 1.0f / GAMMA);

    // Clamp to sensible bounds
    if (!isfinite(lux) || lux < 0.0f)
        lux = 0.0f;
    if (lux > 65535.0f)
        lux = 65535.0f;

    l = lux;
#else
    // --- REAL HARDWARE SENSOR READ ---
    // Read from the real BH1750
    l = lightMeter.readLightLevel();
#endif

    return true;
}