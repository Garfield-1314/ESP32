#ifndef CONFIG_H_
#define CONFIG_H_

//SERIAL SETTINGS
#define BAUD 115200

//ROBOT ARM LENGTH
#define SHANK_LENGTH 140.0
#define END_EFFECTOR_OFFSET 55.0 // LENGTH FROM UPPER SHANK BEARING TO MIDPOINT OF END EFFECTOR IN MM

//INITIAL INTERPOLATION SETTINGS
//  INITIAL_XYZ FORMS VERTICAL LOWER ARM & HORIZONTAL UPPER ARM IN 90 DEGREES
#define INITIAL_X 0.0 // CARTESIAN COORDINATE X  
#define INITIAL_Y (SHANK_LENGTH+END_EFFECTOR_OFFSET) // CARTESIAN COORDINATE Y
#define INITIAL_Z SHANK_LENGTH // CARTESIAN COORDINATE Z
#define INITIAL_E0 0.0 // RAIL STEPPER ENDSTOP POSITION 

//  CALIBRATE HOME STEPS TO REACH DESIRED INITIAL_XYZ POSITIONS
#define X_HOME_STEPS 765 //860 // STEPS FROM X_ENDSTOP TO INITIAL_XYZ FOR UPPER ARM
#define Y_HOME_STEPS 1750 //1940 // STEPS FROM Y_ENDSTOP TO INITIAL_XYZ FOR LOWER ARM
#define Z_HOME_STEPS 3721 // STEPS FROM Z_ENDSTOP TO INITIAL_XYZ FOR ROTATION CENTER
#define E0_HOME_STEPS 0 // STEPS FROM E0_ENDSTOP TO INITIAL_E0

//HOMING SETTINGS:
#define HOME_X_STEPPER true // "true" IF ENDSTOP IS INSTALLED
#define HOME_Y_STEPPER true // "true" IF ENDSTOP IS INSTALLED
#define HOME_Z_STEPPER true // "true" IF ENDSTOP IS INSTALLED
#define HOME_E0_STEPPER true // "true" IF ENDSTOP IS INSTALLED
#define HOME_ON_BOOT false // "true" IF HOMING REQUIRED AFTER POWER ON
#define HOME_DWELL 1400 // INCREASE TO SLOW DOWN HOMING SPEED

//STEPPER SETTINGS:
#define MICROSTEPS 16 // MICROSTEPPING CONFIGURATION ON RAMPS1.4
#define STEPS_PER_REV 200 // NEMA17 STEPS PER REVOLUTION
#define INVERSE_X_STEPPER true // CHANGE IF STEPPER MOVES OTHER WAY
#define INVERSE_Y_STEPPER false // CHANGE IF STEPPER MOVES OTHER WAY
#define INVERSE_Z_STEPPER true // CHANGE IF STEPPER MOVES OTHER WAY
#define INVERSE_E0_STEPPER false // CHANGE IF STEPPER MOVES OTHER WAY

//RAIL SETTINGS:
#define RAIL true // E0 STEPPER USED AS RAIL. SET TO 'false' IF ROBOT ARM IS STATIONARY.
#define STEPS_PER_MM_RAIL 80.0 // STEPS PER MM FOR RAIL MOTOR
#define RAIL_LENGTH 200.0 // MAX LENGTH OF RAIL IN MM

//ENDSTOP SETTINGS:
#define X_MIN_INPUT 1 // OUTPUT VALUE WHEN SWITCH ACTIVATED
#define Y_MIN_INPUT 1 // OUTPUT VALUE WHEN SWITCH ACTIVATED
#define Z_MIN_INPUT 1 // OUTPUT VALUE WHEN SWITCH ACTIVATED
#define E0_MIN_INPUT 1 // OUTPUT VALUE WHEN SWITCH ACTIVATED

//GEAR RATIO SETTINGS
#define MOTOR_GEAR_TEETH 20.0 // 20.0 FOR 20SFFACTORY BELT VERSION   9.0 FOR FTOBLER GEAR VERSION
#define MAIN_GEAR_TEETH 90.0 // 90.0 FOR 20SFFACTORY BELT VERSION   32.0 FOR FTOBLER GEAR VERSION

//EQUIPMENT SETTINGS
#define LASER false // 12V LASER CONNECTED TO LASER_PIN
#define PUMP false // 12V AIR PUMP CONNECTED TO PUMP_PIN
#define FAN_DELAY 120 // FAN ON IN SECONDS

//28BYJ GRIPPER SETTINGS
#define GRIP_STEPS 1200 //FTOBLER: 1200

//COMMAND QUEUE SETTINGS
#define QUEUE_SIZE 15

//PRINT REPLY SETTING
#define PRINT_REPLY false // "true" TO PRINT MSG AFTER ONE COMMAND IS PROCESSED
#define PRINT_REPLY_MSG "ok" // MSG SENT FOR USER'S POST PROCESSING WITH OTHER SOFTWARE

//SPEED PROFILE SETTING
#define SPEED_PROFILE 2 // OPTIONS BELOW
//0: FLAT SPEED CURVE (CONSTANT SPEED PER MOVEMENT, SUITABLE FOR REALTIME CONTROL SOFTWARE)
//1: ARCTAN APPROX (SLIGHT BELL CURVE ACCELERATION & DECELERATION)
//2: COSIN APPROX (TOTAL BELL CURVE ACCEL FROM 0 & DECEL FROM 0, SUITABLE FOR PRESET COMMAND MOVEMENTS)

//LOG SETTINGS
#define LOG_LEVEL 2
//0: ERROR
//1: INFO
//2: DEBUG

//MOVE LIMIT PARAMETERS
#define Z_MIN -125.0 //MINIMUM Z HEIGHT OF TOOLHEAD TOUCHING GROUND
#define Z_MAX (SHANK_LENGTH+30.0) //SHANK_LENGTH ADDING ARBITUARY NUMBER FOR Z_MAX
#define R_MIN (SHANK_LENGTH * 0.85 + END_EFFECTOR_OFFSET) //INNER RADIUS MINIMUM. x0.85 APPROX OF LAW OF COSINES
#define R_MAX (SHANK_LENGTH * 1.85 + END_EFFECTOR_OFFSET) //OUTTER RADIUS MAXIMUM. x1.85 APPROX OF LAW OF COSINES

#endif
