#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>

// Pin definition
#define SENSOR_STATE_INDICATOR 39

// Globals
extern bool ready_sensor[100];

// Functions
void set_sensor_state(uint8_t sensor, uint8_t state);

#endif // STATEMANAGER_H
