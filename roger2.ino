#include <ZumoMotors.h>

/*

roger2

*/

const int PIN_LED = 13;


ZumoMotors motor;


void setup() {
    pinMode(PIN_LED, OUTPUT);
}


void loop() {
    int motorSpeed = 200;
    motor.setLeftSpeed(motorSpeed);
    motor.setRightSpeed(motorSpeed);
    delay(3000);
    motorSpeed = 0;
    motor.setLeftSpeed(motorSpeed);
    motor.setRightSpeed(motorSpeed);
    motorSpeed = -400;
    motor.setLeftSpeed(motorSpeed);
    motor.setRightSpeed(motorSpeed);
    delay(2000);
    motorSpeed = 0;
    motor.setLeftSpeed(motorSpeed);
    motor.setRightSpeed(motorSpeed);
    delay(6000);
}

