#include <Arduino.h>
#define SENSOR_STATE_INDICATOR 39

bool ready_sensor[100] = { 0 };
void set_sensor_state(uint8_t sensor, uint8_t state) {
    if (!ready_sensor[sensor]) {
        pinMode(sensor, OUTPUT);
        ready_sensor[sensor] = true;
    }

    digitalWrite(sensor, state);
}