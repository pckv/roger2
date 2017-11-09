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
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>

const int PIN_LED = 13;

const int NUM_SEONSORS = 6;

bool logging = true;

enum Direction {Straight, Left, Right, SwivelLeft, SwivelRight};


ZumoMotors motor;

unsigned int sensorValues[NUM_SEONSORS];
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
}


void printSensorValues() {
    Serial.print("Sensors: {");
    for (int i = 0; i < NUM_SEONSORS; i++) {
        Serial.print(sensorValues[i]);
        Serial.print(", ");
    }
    Serial.println("}");
}


void loop() {
    // Store sensor readings in sensorValues
    sensors.read(sensorValues);

    if (logging) {
        printSensorValues();
    }

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
