#include <Arduino.h>
#include <Servo.h>
#include <Controller_Main.h>
#include <Adafruit_MotorShield.h>

Servo servo;
const int MAX_LEFT_ANGLE = 0;
const int MAX_RIGHT_ANGLE = 180;

// Engine configuration
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *motor = AFMS.getMotor(1);

void setup() {
  Serial.begin(9600);
  AFMS.begin();

  // Servo motor
  servo.attach(8);

  // Time of flight sensors
  load_sensors();
  motor->setSpeed(0);
}

void loop() {
  // Reads and updates sensor data
  motor->run(FORWARD);
  read_sensor_data();

  float angle = turning_angle;
  if (angle > MAX_RIGHT_ANGLE)
    angle = MAX_RIGHT_ANGLE;
  if (angle < MAX_LEFT_ANGLE)
    angle = MAX_LEFT_ANGLE;

  servo.write(angle);
}
