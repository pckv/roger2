/*
 * Roger 2
 *
 * TODO: Bruk accelerometer til å vite om vi har blitt eid av motstanderen
 *
*/

// https://github.com/pololu/zumo-shield
#include <ZumoMotors.h>
#include <ZumoBuzzer.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>

// https://github.com/DrGFreeman/SharpDistSensor
#include <SharpDistSensor.h>

// Inklude the header file containing music note sheet
#include "notes.h"


// Declare pins
const int PIN_LED = 13;
const int PIN_SENSOR_IR_LEFT = A1;
const int PIN_SENSOR_IR_RIGHT = A2;

// Declare global constants
const int SENSOR_SAMPLE_SIZE = 6; // The sensor returns the mean value of x amount of samples

const int NUM_SENSORS = 6;  // Number of sensors in the array
const int MAX_BORDER_SENSOR_RANGE = 2000;  // Highest value for border sensors
const int WHITE_THRESHOLD = 1920;  // For the light sensors
const int TARGET_DISTANCE_THRESHOLD = 420;  // For the front sensors

const int MAX_SPEED = 400;
const int CASUAL_SPEED = 240;
const int TURN_SPEED = 250;

const int MAX_IR_SENSOR_DIFFERENCE = 200;  // The highest measured distance for targeting
const int IR_SENSOR_STRAIGHT_RATIO = 30;  // Ratio (out of 100) for when the targeting is deemed straight ahead

const int TARGET_DRIVE_INTERVAL = 20;  // Change driving every 30ms when attacking (for fine tuning)
const int REVERSE_DURATION = 500;
const int TURN_TIMER_DURATION = 1300;
const int TURN_TIMER_INTERVAL = 1300;
const int STARTUP_SLEEP_TIME = 2000;  // As per the rules TODO: change to 5000

// Declare global variables
unsigned long actionStarted;  // Store the time action states changed.
unsigned long startedTimers[8];  // TODO: set to size of Timer enum

bool logging = false;  // Debug logs on Serial port 9600  TODO: Disable logging before end of Roger 2 dev period
bool playingMusic = false;

// Make enums
enum Direction {
    None,
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
    Turn,       // After Retreat, for turning around
    // TODO: add "crisis" state for when we're literally stuck in Destroy
    Victory     // Warp to dance floor
};

// Timer IDs for using multiple timers at once
enum Timer {
    StartupTimer,
    SearchTimer,
    TargetDriveIntervalTimer,
    TurnTimer
};

// Make the sensor and motor objects
SharpDistSensor sensorIRLeft(PIN_SENSOR_IR_LEFT, SENSOR_SAMPLE_SIZE);
SharpDistSensor sensorIRRight(PIN_SENSOR_IR_RIGHT, SENSOR_SAMPLE_SIZE);

ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);
ZumoMotors motor;
ZumoBuzzer buzzer;

unsigned int currentMelodyIndex = 0;

// Set the default ActionState
ActionState actionState = Startup;
Direction lastActionBorderSensor;


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);

    // These are set default to GP2Y0A60SZLF_5V in SharpDistSensor
    // sensorIRLeft.setModel(SharpDistSensor::GP2Y0A60SZLF_5V);
    // sensorIRRight.setModel(SharpDistSensor::GP2Y0A60SZLF_5V);

    // Start a 5 second timer before changing to a bigboi state
    startTimer(StartupTimer, STARTUP_SLEEP_TIME);
}


/*
 * Get the duration of the current ActionState.
 *
 * @return Time in ms since the ActionState switched.
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
 * Check if a timer has expired.
 *
 * @param timer Which Timer ID to check.
 * @return True if the timer has expired.
 */
bool hasTimerExpired(Timer timer) {
    return startedTimers[timer] <= millis();
}


/*
 * Inverts the given Direction: Left -> Right, SwivelLeft -> SwivelRight etc.
 *
 * @param direction The direction to invert.
 * @returns The inverted Direction.
 */
Direction invertDirection(Direction direction) {
    switch (direction) {
        case Left: return Right;
        case Right: return Left;
        case SwivelLeft: return SwivelRight;
        case SwivelRight: return SwivelLeft;
        default: return direction;
    }
}


/*
 * Print read border sensor values as:
 * Sensors: {#0, #1, #2, #3, #4, #5}
 *
 * @param values Array of values to print.
 */
void printBorderSensorValues(unsigned int values[NUM_SENSORS]) {
    Serial.print("Sensors: {");
    for (unsigned int i = 0; i < NUM_SENSORS; i++) {
        Serial.print(values[i]);
        Serial.print(", ");
    }
    Serial.println("}");
}


/*
 * Print IR sensor values as:
 * Left: #Left,  Right: #Right
 *
 * @param valueLeft Measured value of the leftmost IR sensor.
 * @param valueRight Measured value of the rightmost IR sensor.
 */
void printIRSensorValues(unsigned int valueLeft, unsigned int valueRight) {
    Serial.print("Left: ");
    Serial.print(valueLeft);
    Serial.print(",  Right: ");
    Serial.println(valueRight);
}


/*
 * Prints the name of the given ActionState.
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
        case Turn:
            Serial.print("Turn");
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
 * The drive function makes roger move in any direction.
 *
 * @param speed The speed of the motors.
 * @param direction The direction to drive.
 * @param turnSpeedOffset For one of the motors when turning. Lower number means a sharper turn.
 */
void drive(int speed, Direction direction = None, float turnSpeedOffset = 1) {
    speed = constrain(speed, -MAX_SPEED, MAX_SPEED);
    turnSpeedOffset = constrain(turnSpeedOffset, -1, 1);

    switch (direction) {
        case None:
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
        default:
            motor.setSpeeds(0, 0);
    }
}


/*
 * Get whichever sensor is above the border of the arena, if any.
 *
 * @param sensorLeft Registered value of the leftmost sensor.
 * @param sensorRight Registered value of the rightmost sensor.
 * @return A Direction Left or Right denoting the left or right sensor, or a Direction None.
 */
Direction getSensorAboveBorder(unsigned int sensorLeft, unsigned int sensorRight) {
    // Return the lowest value sensor
    if (sensorRight < sensorLeft) {
        return Right;
    }
    else if (sensorRight > sensorLeft) {
        return Left;
    }

    // If the sensor values are equal, they are *highly* likely to both be 2000, so we can assume it's on not the border
    else {
        return None;
    }
}


/*
 * Get whichever IR sensor that sees a target closest, if any.
 *
 * A target is deemed seen when the sensor measures a value below the TARGET_DISTANCE_THRESHOLD.
 *
 * @param valueLeft Measured value of the leftmost IR sensor.
 * @param valueRight Measured value of the rightmost IR sensor.
 * @returns A Direction Left, Right or None.
 */
Direction getIRSensorTarget(unsigned int valueLeft, unsigned int valueRight) {
    // Return None if no sensor sees a target
    if ((valueLeft > TARGET_DISTANCE_THRESHOLD) && (valueRight > TARGET_DISTANCE_THRESHOLD)) {
        return None;
    }

    // Return the lowest value sensor
    else if (valueLeft < valueRight) {
        return Left;
    }
    else {
        return Right;
    }
}


/*
 * Get the offset needed for turning towards a target.
 *
 * Closer values means offset closer to 1, while values further apart give a lower offset, such as 0.5
 *
 * This function should only be used when a target is deemed in front of the bot.
 *
 * @param valueLeft Measured value of the leftmost IR sensor.
 * @param valueRight Measured value of the rightmost IR sensor.
 * @returns The turning offset multiplier.
 */
float getIRSensorOffset(unsigned int valueLeft, unsigned int valueRight) {
    // Check the absolute difference between the sensors:
    unsigned int difference = abs(valueLeft - valueRight);

    // Constrain the difference within a maximum
    difference = constrain(difference, 0, MAX_IR_SENSOR_DIFFERENCE);

    // Map the difference between 0 and 100 for more usable values later
    difference = map(difference, 0, MAX_IR_SENSOR_DIFFERENCE, 0, 100);

    // If the difference is small enough, pretend the target is straight ahead
    if (difference < IR_SENSOR_STRAIGHT_RATIO) {
        return 1;
    }
    else {
        // Calculate the offset based on a map from 0 to 70
        // It's important to invert the offset (100 - map...), since a lower difference should be closer to 1, not 0
        float offset = 100 - map(difference, IR_SENSOR_STRAIGHT_RATIO, 100, 0, 70);

        // Divide the offset by 100 and return the float in the domain [0, 1]
        return offset / 100;
    }
}


/*
 * Changes state to Search and starts driving.
 *
 * @param borderSensor The currently measured sensor Direction.
 */
void initiateSearch(Direction borderSensor) {
    drive(CASUAL_SPEED, invertDirection(lastActionBorderSensor), (float) (random(80, 100)) / 100);
    changeState(Search);
    startTimer(TurnTimer, TURN_TIMER_INTERVAL);
}


/*
 * Changes state to Retreat and starts driving.
 *
 * @param borderSensor The currently measured sensor Direction.
 */
void initiateRetreat(Direction borderSensor) {
    drive(-MAX_SPEED, invertDirection(borderSensor), 0.6);
    changeState(Retreat);
    lastActionBorderSensor = borderSensor;
}


void playNote() {
    if (currentMelodyIndex < MELODY_LENGTH && !buzzer.isPlaying())
    {
        // play note at max volume
        buzzer.playNote(melodyNotes[currentMelodyIndex], noteDuration, 10);
        currentMelodyIndex++;
    }
}


void loop() {
    unsigned int sensorValues[NUM_SENSORS];  // TODO: Register only the two side sensors with pin 4 and 5.
    unsigned int sensorIRLeftValue, sensorIRRightValue;

    // Store sensor readings in sensorValues
    sensors.read(sensorValues);

    // Measure front facing IR sensor values
    sensorIRLeftValue = getSensorDistance(sensorIRLeft);
    sensorIRRightValue = getSensorDistance(sensorIRRight);

    // Always find which sensor is above the border, if any
    Direction borderSensor = getSensorAboveBorder(sensorValues[0], sensorValues[5]);

    if (playingMusic) {
        playNote();
    }

    if (logging) {
        // printSensorValues(sensorValues);
        printIRSensorValues(sensorIRLeftValue, sensorIRRightValue);
    }

    switch (actionState) {
        case Startup:
            if (hasTimerExpired(StartupTimer)) {
                initiateSearch(borderSensor);
            }
            break;

        case Search:
            // Change to Retreat when sensors are above the border
            if (borderSensor != None) {
                initiateRetreat(borderSensor);
            }
            else if (getIRSensorTarget(sensorIRLeftValue, sensorIRRightValue) != None) {
                changeState(Destroy);
            }
            else if (hasTimerExpired(TurnTimer)) {
                startTimer(TurnTimer, TURN_TIMER_DURATION);
                changeState(Turn);
                drive(TURN_SPEED, millis() % 2 == 0 ? SwivelLeft : SwivelRight);
            }
            break;

        case Destroy:
            if (borderSensor != None) {
                initiateRetreat(borderSensor);
            }
            else {
                Direction targetDirection = getIRSensorTarget(sensorIRLeftValue, sensorIRRightValue);

                if (targetDirection == None) {
                    initiateSearch(borderSensor);
                }
                else { // if (hasTimerExpired(TargetDriveIntervalTimer)) {
                    float turnOffset = getIRSensorOffset(sensorIRLeftValue, sensorIRRightValue);
                    drive(MAX_SPEED, targetDirection, turnOffset);
                    // startTimer(TargetDriveIntervalTimer, TARGET_DRIVE_INTERVAL);
                }
            }
            break;

        case Retreat:
            // After REVERSE_DURATION ms, and if the sensors are not above the border, go back to search mode
            if ((getActionDuration() >= REVERSE_DURATION) && (borderSensor == None)) {
                drive(TURN_SPEED, lastActionBorderSensor == Left ? SwivelRight : SwivelLeft);
                changeState(Turn);
                startTimer(TurnTimer, 400);
            }
            break;

        case Turn:
            if (getIRSensorTarget(sensorIRLeftValue, sensorIRRightValue) != None) {
                changeState(Destroy);
            }
            else if (hasTimerExpired(TurnTimer)) {                
                initiateSearch(borderSensor);
            }
            break;

        case Victory:
            break;
    }
}


/*
 * Get distance from IR sensor in millimeters.
 *
 * @params sensor is the sensor we want to use.
 * @return the distance in millimeters.
 */
unsigned int getSensorDistance(SharpDistSensor &sensor) {
    return sensor.getDist();
}
