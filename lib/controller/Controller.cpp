#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <avr/wdt.h>
#include <Sorting.h>
#include <Controller.h>

const int SENSOR_DISTANCE = 135;
const double Kp = 0.2;
const double Kd = 0.72;
const int STRAIGHT_ANGLE = 90;
const int VEHICLE_WIDTH = 95;

// Data from sensors
float f_left = 0, 
    f_right = 0, 
    b_left = 0, 
    b_right = 0, 
    front = 0;

Adafruit_VL53L1X* SENSORS[5];
const int XSHUT_PINS[5] = { 2, 3, 4, 5, 6 };
const byte SENSOR_ADDRESS[5] = { 0x30, 0x31, 0x32, 0x33, 0x34 };
const int MEASUREMENT_ERROR[5] = { 10, 30, 0, -5, 10  };

bool start_sensor(int index, Adafruit_VL53L1X* sensor) {
    if (sensor->begin(SENSOR_ADDRESS[index], &Wire)) {
        if (!sensor->startRanging()) {
            Serial.println("Nepavyko pradeti atstumo matavimo");
            return false;
        }

        Serial.println(String("Pavyko paleisti sensoriu ") + index);
        sensor->setTimingBudget(60);
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

        delay(100);
    }
}

// Servo turning angle
const size_t buffer_size = 10;
float turning_angle = STRAIGHT_ANGLE;
static unsigned long last_time = millis();
float last_error;
float track_buffer[buffer_size];
int track_tracker = 0;
bool track_ready = false;

void read_sensor_data() {
    // Real-time sensor data reading
    float distances[5] = { 0 };
    for (int i = 0; i < 5; i++) {
        if (SENSORS[i]->dataReady()) {
            const float distance = SENSORS[i]->distance();
            distances[i] = distance;
            SENSORS[i]->clearInterrupt();
        }

        delay(30);
    }

    front = distances[3] + MEASUREMENT_ERROR[0];
    f_left = distances[4] + MEASUREMENT_ERROR[1];
    f_right = distances[2] + MEASUREMENT_ERROR[2];
    b_left = distances[1] + MEASUREMENT_ERROR[3];
    b_right = constrain(distances[0] + MEASUREMENT_ERROR[4], 0, 2000);

    /*  PID controller
        We are calculating the angle to the outside wall.
        Initially, we assume the outside wall is on the left side of the robot.
        However, if the left distance changes rapidly, we start tracking the right side of the robot.
    */

    float track_x1 = f_right;
    float track_x2 = b_right;
    const float wall_angle = atan((track_x1 - track_x2) / SENSOR_DISTANCE);
    const float wall_distance = ((track_x1 + track_x2) / 2) * cos(wall_angle);

    // Calculating the distance between walls
    const float size = ((f_left + f_right + b_left + b_right) / 2) * cos(wall_angle) + VEHICLE_WIDTH;
    if (track_tracker == (buffer_size - 1)) {
        track_tracker = 0;
    } else {
        track_tracker++;
    }

    track_buffer[track_tracker] = size;
    const float track = get_dominant_cluster_average(
        buffer_size, track_buffer, 20
    );
    
    if (track > 0 && !track_ready) 
        track_ready = true;


    // PID logic
    const float error = track / 2 - wall_distance - VEHICLE_WIDTH/2;
    if (!last_error) {
        last_error = error;
    }

    unsigned long now = millis();
    float delta_t = (now - last_time) / 1000.0f;
    last_time = now;

    const float derivative_delta = (error - last_error) / delta_t;
    last_error = error;

    turning_angle = STRAIGHT_ANGLE 
        - Kp * error
        - Kd * derivative_delta;
    
    front *= cos(wall_angle);
    
    //Serial.print(Kp * error);
    //Serial.print(Kd * derivative_delta);
    Serial.print(front);
    Serial.print(" ");
    Serial.print(0);
    Serial.print(" ");
        Serial.print(0);
    Serial.print(" ");
    Serial.print(front);
    Serial.println();
    delay(30);
}
