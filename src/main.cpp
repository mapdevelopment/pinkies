#include <Arduino.h>
#include <Servo.h>
#include <Controller.h>
#include <Adafruit_MotorShield.h>

#define FAR_DISTANCE -1

Servo servo;
const int MAX_LEFT_ANGLE = 0;
const int MAX_RIGHT_ANGLE = 180;
const int BACK_DISTANCE = 350;

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

  Serial.println(String(f_left) + " " + String(f_right));

  // Turning logic
  if ((f_left + f_right) < 1200 
    && f_left != FAR_DISTANCE 
    && f_right != FAR_DISTANCE
  ) {
    servo.write(angle);
    return;
  }

  angle = MAX_RIGHT_ANGLE;
  if (
    ((f_left > f_right) && f_right != -1) || 
    f_left == FAR_DISTANCE
  ) {
    angle = MAX_LEFT_ANGLE;
  }

  Serial.println((f_left > f_right));

  servo.write(angle);
  if (front < BACK_DISTANCE) {
    servo.write(
      angle == MAX_LEFT_ANGLE ? MAX_RIGHT_ANGLE : MAX_LEFT_ANGLE
    );
    motor->run(BACKWARD);
    delay(1400);
    motor->run(FORWARD);
    servo.write(angle);
    delay(2300);
  }
}
