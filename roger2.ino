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


// Declare pins
const int PIN_LED = 13;
const int PIN_SENSOR_IR_LEFT = A1;
const int PIN_SENSOR_IR_RIGHT = A4;


// Declare global constants
const int SENSOR_SAMPLE_SIZE = 5;   // The sensor returns the mean value of x amount of samples

const int NUM_SENSORS = 6;          // Number of sensors in the array
const int WHITE_THRESHOLD = 1920;   // For the light sensors

const unsigned long STARTUP_SLEEP_TIME = 3000;  // As per the rules TODO: change to 5000

// Declare global variables
unsigned long actionStarted;  // Store the time action states changed.
unsigned long startedTimers[8];  // TODO: set to correct size

bool logging = true;  // Debug logs on Serial port 9600

// Make enums
enum Direction {
    Straight,
    Left,
    Right,
    SwivelLeft,
    SwivelRight
};

enum ActionState {
    Startup,    // Idle for five seconds before starting and entertain audience (zumo rules)
    Search,     // Search for the opponent, usually the state active before targeting and attacking
    Destroy,    // After targeting the opponent, this state is for charging and attacking
    Retreat,    // The Retreat state is active when the robot has reached the edge of the arena
    Victory     // Warp to dance floor
};

enum Timer {
    StartupTimer,
    RetreatTimer
};

// Make the sensor and motor objects
SharpDistSensor sensorIrLeft(PIN_SENSOR_IR_LEFT, SENSOR_SAMPLE_SIZE);
SharpDistSensor sensorIrRight(PIN_SENSOR_IR_RIGHT, SENSOR_SAMPLE_SIZE);

ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);
ZumoMotors motor;

// Set the default ActionState
ActionState actionState = Startup;


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
    sensorA.setModel(SharpDistSensor::GP2Y0A60SZLF_5V);

    // Start a 5 second timer before changing to a bigboi state
    startTimer(StartupTimer, STARTUP_SLEEP_TIME);
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
    return startedTimers[timer] <= millis();
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
    actionStarted = millis();
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

    // unsigned int sensorLeft, sensorRight = sensorValues[0], sensorValues[5];

    if (logging) {
        printSensorValues(sensorValues);
    }

    switch (actionState) {
        case Startup:
            if (hasTimerExpired(StartupTimer)) {
                changeState(Search);
            }
            break;

        case Search:
            if (isSensorAboveBorder(sensorValues, 0)) {
                drive(200, SwivelRight);
                changeState(Retreat);
            }

            if (isSensorAboveBorder(sensorValues, 5)) {
                drive(200, SwivelLeft);
                changeState(Retreat);
            }
            break;

        case Destroy:
            break;

        case Retreat:
            if (getActionDuration() >= 200) {
                drive(200, millis() % 2 == 0 ? Right : Left, (float) (random(75, 100)) / 100);
                changeState(Search);
            }
            break;

        case Victory:
            break;
    }
}


/*
 * Get distance from IR sensor in millimeters
 *
 * @params sensor is the sensor we want to use
 * @return the distance in millimeters
 */
unsigned int getSensorDistance(SharpDistSensor &sensor) {
    return sensor.getDist();
}
