#include <Arduino.h>
#include <Servo.h>
#include <Controller.h>
#include <Adafruit_MotorShield.h>

Servo servo;
const int MAX_LEFT_ANGLE = 0;
const int MAX_RIGHT_ANGLE = 180;
const int BACK_DISTANCE = 200;

// Engine configuration
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *motor = AFMS.getMotor(1);

void setup() {
  Serial.begin(115200);
  AFMS.begin();

  // Servo motor
  servo.attach(8);

  // Time of flight sensors
  load_sensors();
}

void loop() {
  // Reads and updates sensor data
  read_sensor_data();

  if (!track_ready) {
    motor->run(RELEASE);
    return;
  }

  motor->run(FORWARD);
  motor->setSpeed(100);
  float angle = constrain(turning_angle, MAX_LEFT_ANGLE, MAX_RIGHT_ANGLE);

  // Turning logic
  if ((f_left + f_right) > 1200) {
    servo.write(angle);
    return;
  }

  angle = MAX_LEFT_ANGLE;
  if (f_left > f_right) {
    angle = MAX_RIGHT_ANGLE;
  }

  if (front <= BACK_DISTANCE) {
    motor->run(BACKWARD);
    angle *= -1;
  }

  servo.write(angle);
  delay(500);
}
