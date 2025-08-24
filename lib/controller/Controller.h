#ifndef CONTROLLER_H

// Function prototypes
void read_sensor_data(void);
void load_sensors(void);

// Constants
#define MOTOR_SPEED 200

// Global variables
extern float turning_angle;
extern bool track_ready;

extern float f_left;
extern float f_right;
extern float b_left;
extern float b_right;
extern float front;
extern float wall_angle;

#endif // CONTROL_H
