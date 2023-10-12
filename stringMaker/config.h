#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG

#ifdef DEBUG
#define debugln(x) Serial.println(F(x))
#define debug(x) Serial.print(F(x))
#define debugVarln(x) Serial.println(x)
#define debugVar(x) Serial.print(x)

#else
#define debugln(x)
#define debug(x)
#define debugVarln(x)
#define debugVar(x)
#endif



// Pin definitions
// Keyboard
#define KEYB_PIN_ROW_A A3
#define KEYB_PIN_ROW_B A0
#define KEYB_PIN_ROW_C A1
#define KEYB_PIN_ROW_D A2
#define KEYB_PIN_COL_1 10
#define KEYB_PIN_COL_2 9
#define KEYB_PIN_COL_3 12
#define KEYB_PIN_COL_4 13

// Joystick
#define JOY_PIN_X A8
#define JOY_PIN_Y A9
#define JOY_PIN_SWITCH 34

// Button
#define BTN_PIN D39

// Motors
#define MOTOR_PIN_A_ENABLED 1
#define MOTOR_PIN_A_STEP 2
#define MOTOR_PIN_A_DIR 3

#define MOTOR_PIN_B_ENABLED 1
#define MOTOR_PIN_B_STEP 2
#define MOTOR_PIN_B_DIR 3

#define MOTOR_PIN_C_ENABLED 1
#define MOTOR_PIN_C_STEP 2
#define MOTOR_PIN_C_DIR 3

#define MOTOR_PIN_D_ENABLED 1
#define MOTOR_PIN_D_STEP 2
#define MOTOR_PIN_D_DIR 3


/*

byte customChar[0] = {
  0b11111,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b11111,
  0b00000
};

byte customChar[1] = {
  0b11000,
  0b11000,
  0b11000,
  0b11111,
  0b11000,
  0b11000,
  0b11000,
  0b00000
};

byte customChar[2] = {
  0b00111,
  0b01100,
  0b11001,
  0b11011,
  0b11001,
  0b01100,
  0b00111,
  0b00000
};

byte customChar[3] = {
  0b11000,
  0b00100,
  0b10010,
  0b11010,
  0b10010,
  0b00100,
  0b11000,
  0b00000
};

byte customChar[4] = {
  0b00011,
  0b00011,
  0b00011,
  0b11111,
  0b00011,
  0b00011,
  0b00011,
  0b00011
};

byte customChar[5] = {
  0b11111,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b11111,
  0b00000
};

byte customChar[6] = {
  0b00011,
  0b00110,
  0b01100,
  0b11000,
  0b01100,
  0b00110,
  0b00011,
  0b00000
};

byte customChar[7] = {
  0b11000,
  0b01100,
  0b00110,
  0b00011,
  0b00110,
  0b01100,
  0b11000,
  0b00000
};
*/

#endif
