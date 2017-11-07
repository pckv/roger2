/*

roger2

*/

#include <ZumoMotors.h>


const int PIN_LED = 13;


enum Direction {Forward, Backward, Left, Right, SwivelLeft, SwivelRight};


ZumoMotors motor;


void setup() {
    pinMode(PIN_LED, OUTPUT);
}


void loop() {
    delay(2000);
    drive(200, Forward);
    delay(2000);
    drive(200, Backward);
    delay(2000);
    drive(200, Left);
    delay(2000);
    drive(400, Right);
    delay(2000);
    drive(200, SwivelLeft);
    delay(2000);
    drive(400, SwivelRight),
    delay(2000);
}

//hei

void drive(int speed, Direction direction) {
    speed = constrain(speed, -400, 400);
    switch (direction) {
        case Forward:
            motor.setLeftSpeed(speed);
            motor.setRightSpeed(speed);
            break;
        case Backward:
            motor.setLeftSpeed(-speed);
            motor.setRightSpeed(-speed);
            break;
        case Left:
            motor.setLeftSpeed(speed * 0.4);
            motor.setRightSpeed(speed);
            break;
        case Right:
            motor.setLeftSpeed(speed);
            motor.setRightSpeed(speed * 0.4);
            break;
        case SwivelLeft:
            motor.setLeftSpeed(-speed);
            motor.setRightSpeed(speed);
            break;
        case SwivelRight:
            motor.setLeftSpeed(speed);
            motor.setRightSpeed(-speed);
        break;
    }
}