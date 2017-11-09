/*
 * Roger 2
 *
 * Ledige pins: 4, 5, (6,) 11, 12, A0, A2, A3
 *
 *
 * TODO: alle punkter under
 *      Finpuss drive funksjon
 *      State machine
 *      Vite når roboten er på kanten av arena
 *      Skrive ut feste til sensor
 *
*/

#include <ZumoMotors.h>


const int PIN_LED = 13;


enum Direction {Straight, Left, Right, SwivelLeft, SwivelRight};



ZumoMotors motor;


void setup() {
    pinMode(PIN_LED, OUTPUT);
}


void loop() {
    testDrive();
}



/*
 * The drive function makes roger move in any direction
 *
 * @param speed is the speed of the motors
 *
 * @param direction the direction to drive
 *
 * @param turnSpeedOffset for one of the motors when turning. Lower number means a sharper turn.
 */
void drive(int speed, Direction direction, float turnSpeedOffset = 1) {
    speed = constrain(speed, -400, 400);
    switch (direction) {
        case Straight:
            motor.setSpeeds(speed, speed);
            break;
        case Left:
            motor.setSpeeds(speed * turnSpeedOffset, speed);
            break;
        case Right:
            motor.setSpeeds(speed, speed * turnSpeedOffset);
            break;
        case SwivelLeft:
            motor.setSpeeds(-speed, speed);
            break;
        case SwivelRight:
            motor.setSpeeds(speed, -speed);
            break;
    }
}


void testDrive() {
    delay(2000);
    drive(200, Straight);
    delay(2000);
    drive(200, Left, 0.5);
    delay(2000);
    drive(400, Right, 0.8);
    delay(2000);
    drive(200, SwivelLeft);
    delay(2000);
    drive(400, SwivelRight);
    delay(2000);
    drive(-200, Right, 0.3);
    delay(2000);
}
