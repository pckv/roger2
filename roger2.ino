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
 *      Bruk accelerometer til å vite om vi har blitt eid av motstanderen
 *
*/

// https://github.com/pololu/zumo-shield
#include <ZumoMotors.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>

// https://github.com/DrGFreeman/SharpDistSensor
#include <SharpDistSensor.h>


const int PIN_LED = 13;

const int NUM_SENSORS = 6;
const int WHITE_THRESHOLD = 1920;

bool logging = true;  // Debug logs on Serial port 9600

enum Direction {Straight, Left, Right, SwivelLeft, SwivelRight};

ZumoMotors motor;
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
}


/*
 * Print read sensor values as:
 * Sensors: {#0, #1, #2, #3, #4, #5}
 *
 * @param values Array of values to print
 */
void printSensorValues(unsigned int values[NUM_SENSORS]) {
    Serial.print("Sensors: {");
    for (unsigned int i = 0; i < NUM_SENSORS; i++) {
        Serial.print(values[i]);
        Serial.print(", ");
    }
    Serial.println("}");
}


/*
 * Check if the given sensor ID is above the arena border.
 *
 * @param values Array of NUM_SENSORS values to check
 * @param id The id of the sensor to check
 */
bool isSensorAboveBorder(unsigned int values[NUM_SENSORS], unsigned int id) {
    return values[id] <= WHITE_THRESHOLD;
}


void loop() {
    unsigned int sensorValues[NUM_SENSORS];

    // Store sensor readings in sensorValues
    sensors.read(sensorValues);

    if (logging) {
        printSensorValues(sensorValues);
    }

    // testDrive();
}


/*
 * The drive function makes roger move in any direction.
 *
 * @param speed is the speed of the motors
 * @param direction the direction to drive
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


/*
 * Prøvekjøring
 */
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
