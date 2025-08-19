void read_sensor_data();
void load_sensors();

#define MOTOR_SPEED 200

extern float turning_angle;
extern bool track_ready;

extern float f_left, 
    f_right, 
    b_left, 
    b_right, 
    front;