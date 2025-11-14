#pragma once

/**
 * @brief Initializes the DHT11 and BH1750 sensors.
 */
void setup_sensors();

/**
 * @brief Reads data from all sensors.
 *
 * @param t Reference to a float to store temperature.
 * @param h Reference to a float to store humidity.
 * @param l Reference to a float to store light level (lux).
 * @return true if all readings were successful, false otherwise.
 */
bool read_all_sensors(float &t, float &h, float &l);