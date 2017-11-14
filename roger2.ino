/*
 * Roger 2
 *
 * TODO: Bruk accelerometer til å vite om vi har blitt eid av motstanderen
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
const int PIN_SENSOR_IR_RIGHT = A2;


// Declare global constants
const int SENSOR_SAMPLE_SIZE = 5; // The sensor returns the mean value of x amount of samples

const int NUM_SENSORS = 6;  // Number of sensors in the array
const int MAX_BORDER_SENSOR_RANGE = 2000;  // Highest value for border sensors
const int WHITE_THRESHOLD = 1920;  // For the light sensors
const int TARGET_DISTANCE_THRESHOLD = 300;  // For the front sensors

const int MAX_SPEED = 400;
const int CASUAL_SPEED = 400;

const unsigned long STARTUP_SLEEP_TIME = 2000;  // As per the rules TODO: change to 5000

// Declare global variables
unsigned long actionStarted;  // Store the time action states changed.
unsigned long startedTimers[8];  // TODO: set to size of Timer enum

bool logging = false;  // Debug logs on Serial port 9600

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
    Victory     // Warp to dance floor
};

// Timer IDs for using multiple timers at once
enum Timer {
    StartupTimer,
    SearchTimer,
    TurnTimer
};

// Make the sensor and motor objects
SharpDistSensor sensorIRLeft(PIN_SENSOR_IR_LEFT, SENSOR_SAMPLE_SIZE);
SharpDistSensor sensorIRRight(PIN_SENSOR_IR_RIGHT, SENSOR_SAMPLE_SIZE);

ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);
ZumoMotors motor;

// Set the default ActionState
ActionState actionState = Startup;
Direction lastActionBorderSensor;


void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
    sensorIRLeft.setModel(SharpDistSensor::GP2Y0A60SZLF_5V);
    sensorIRRight.setModel(SharpDistSensor::GP2Y0A60SZLF_5V);

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

    // If the sensor values are equal, they are *highly* likely to both be 2000, so we can assume it's on the border
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
 * This function should only be used when a target is deemed in front of the bot.
 *
 * @param valueLeft Measured value of the leftmost IR sensor.
 * @param valueRight Measured value of the rightmost IR sensor.
 * @returns The turning offset multiplier.
 */
float getIRSensorOffset(unsigned int valueLeft, unsigned int valueRight) {
    // Check the difference between the sensors:
    // closer values means offset closer to 1, while values further apart give a lower offset, such as 0.5
    unsigned int difference = abs(valueLeft - valueRight);
    difference = constrain(difference, 0, 200) / 2;

    if (difference < 30) {
        return 1;
    }
    else {
        float offset = 100 - map(difference, 30, 100, 0, 70);
        return offset / 100;
    }

    /*difference = constrain(difference, 30, 200);
    float offset = map(difference, 30, 200, 20, 70);
    return offset / 100;*/
}


/*
 * Changes state to Search and starts driving.
 *
 * @param borderSensor The currently measured sensor Direction.
 */
void initiateSearch(Direction borderSensor) {
    drive(CASUAL_SPEED, invertDirection(lastActionBorderSensor), (float) (random(80, 100)) / 100);
    changeState(Search);
    startTimer(TurnTimer, 700);
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
                startTimer(TurnTimer, 700);
                changeState(Turn);
                drive(MAX_SPEED, millis() % 2 == 0 ? SwivelLeft : SwivelRight);
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
                else {
                    float turnOffset = getIRSensorOffset(sensorIRLeftValue, sensorIRRightValue);
                    drive(MAX_SPEED, targetDirection, turnOffset);
                }
            }
            break;

        case Retreat:
            // After 200ms, and if the sensors are not above the border, go back to search mode
            if ((getActionDuration() >= 300) && (borderSensor == None)) {
                drive(MAX_SPEED, lastActionBorderSensor == Left ? SwivelRight : SwivelLeft);
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
