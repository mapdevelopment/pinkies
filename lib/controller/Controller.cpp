#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <avr/wdt.h>

const int SENSOR_DISTANCE = 135;
const double PROPORTIONAL_CONST = 0.3;
const double DERIVATIVE_CONST = 0;
const int STRAIGHT_ANGLE = 90;

// Data from sensors
float f_left = 0, 
    f_right = 0, 
    b_left = 0, 
    b_right = 0, 
    front = 0;

Adafruit_VL53L1X* SENSORS[5];
const int XSHUT_PINS[5] = { 2, 3, 4, 5, 6 };
const byte SENSOR_ADDRESS[5] = { 0x30, 0x31, 0x32, 0x33, 0x34 };
const int MEASUREMENT_ERROR[5] = { 10, 25, 0, -8, 0 };

bool start_sensor(int index, Adafruit_VL53L1X* sensor) {
    if (sensor->begin(SENSOR_ADDRESS[index], &Wire)) {
        if (!sensor->startRanging()) {
            Serial.println("Nepavyko pradeti atstumo matavimo");
            return false;
        }

        Serial.println(String("Pavyko paleisti sensoriu ") + index);
        sensor->setTimingBudget(50);
        return true;
    }

    return false;
}

void load_sensors() {
    Wire.begin();
    Wire.setTimeout(100);

    // Let's assign unique I2C address for each sensor
    for (int i = 0; i < 5; i++) {
        SENSORS[i] = new Adafruit_VL53L1X(XSHUT_PINS[i]);
        if (!start_sensor(i, SENSORS[i]))  {
            Serial.println("Nepavyko paleisti sensorio");
        }

        delay(50);
    }
}

// Servo turning angle
float turning_angle = STRAIGHT_ANGLE;
float last_error;

void read_sensor_data() {
    // Real-time sensor data reading
    float distances[5] = { 0 };
    for (int i = 0; i < 5; i++) {
        //Serial.println("prasom datos");
        if (SENSORS[i]->dataReady()) {
            const float distance = SENSORS[i]->distance();
            distances[i] = distance;
            SENSORS[i]->clearInterrupt();
            
            Serial.print(i);
            Serial.print(" ");
            Serial.print(distance);
            Serial.println();
        }

        delay(50);
    }

    Serial.println();
    front = distances[3] + MEASUREMENT_ERROR[0];
    f_left = distances[4] + MEASUREMENT_ERROR[1];
    f_right = distances[2] + MEASUREMENT_ERROR[2];
    b_left = distances[1] + MEASUREMENT_ERROR[3];
    b_right = distances[0] + MEASUREMENT_ERROR[4];    

    // PID controller
    // We are calculating the angle to the outside wall.
    // Initially, we assume the outside wall is on the left side of the robot.
    // However, if the left distance changes rapidly, we start tracking the right side of the robot.

    float track_x1 = f_left;
    float track_x2 = b_left;
    const float wall_angle = atan((track_x1 - track_x2) / SENSOR_DISTANCE);
    const float wall_distance = ((track_x1 + track_x2) / 2) * cos(wall_angle);
    const float track_size = ((f_left + f_right + b_left + b_right) / 2) * cos(wall_angle);
    const float error = track_size / 2 - wall_distance;
    if (!last_error) {
        last_error = error;
    }

    const float derivative_delta = (error - last_error);

    turning_angle = 
        STRAIGHT_ANGLE 
        + PROPORTIONAL_CONST * error
        + DERIVATIVE_CONST * derivative_delta;

    /*
    Serial.print(0);
    Serial.print(" ");
    Serial.print(PROPORTIONAL_CONST * error);
    Serial.print(" ");
    Serial.print(DERIVATIVE_CONST * derivative_delta);
    Serial.print(" ");
    Serial.print(track_size);
    Serial.println();*/
}
