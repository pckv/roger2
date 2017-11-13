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

const int STARTUP_SLEEP_TIME = 5000;  // As per the rules

bool logging = true;  // Debug logs on Serial port 9600

enum Direction {Straight, Left, Right, SwivelLeft, SwivelRight};

enum ActionState {
    Startup,    // Idle for five seconds before starting and entertain audience (zumo rules)
    Search,     // Search for the opponent, usually the state active before targeting and attacking
    Destroy,    // After targeting the opponent, this state is for charging and attacking
    Retreat,    // The Retreat state is active when the robot has reached the edge of the arena
    Victory     // Warp to dance floor
};

unsigned long actionStarted;  // Store the time action states changed.

ActionState actionState = Startup;

enum Timer {
    StartupTimer,
};

unsigned long startedTimers[1];  // TODO: Replace 1 with updated size of Timer

ZumoMotors motor;
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
}


/*
 * Return how long the current state has been active.
 */
unsigned long getActionDuration() {
    return millis() - actionStarted;
}


/*
 * Starts a timer with the given Timer id.
 *
 * @param Timer Which timer to start.
 * @param duration When the timer should expire.
 */
void startTimer(Timer timer, unsigned long duration) {
    startedTimers[timer] = millis() + duration;
}


/*
 * Returns whether the given timer has expired.
 *
 * @param Timer Which Timer to check.
 */
bool hasTimerExpired(Timer timer) {
    return startedTimers[timer] >= millis();
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
 * Prints the name of the given ActionState
 *
 * @param state ActionState to print.
 */
void printActionState(ActionState state) {
    switch (state) {
        case Startup:
            Serial.print("Startup");
            break;
        case Search:
            Serial.print("Search");
            break;
        case Destroy:
            Serial.print("Destroy");
            break;
        case Retreat:
            Serial.print("Retreat");
            break;
        case Victory:
            Serial.print("Victory");
            break;
        default:
            Serial.print("UNKNOWN");
    }
}


/*
 * Change the action state.
 *
 * @param newState The ActionState to change to.
 */
void changeState(ActionState newState) {
    if (logging) {
        Serial.print("Changing Action state from ");
        printActionState(actionState);
        Serial.print(" to ");
        printActionState(newState);
        Serial.println();
    }

    actionState = newState;
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

    unsigned int sensorLeft, sensorRight = sensorValues[0], sensorValues[5];

    if (logging) {
        printSensorValues(sensorValues);
    }


}


/*
 * The drive function makes roger in sonic speeds move in any direction.
 *
 * @param speed The speed of the motors
 * @param direction The direction to drive
 * @param turnSpeedOffset For one of the motors when turning. Lower number means a sharper turn.
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
