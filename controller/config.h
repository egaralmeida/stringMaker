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

#define ROW_A 0
#define ROW_B 1
#define ROW_C 2
#define ROW_D 3

// Keyboard Uno Pin Definitions (remapped)
#define KEYB_PIN_ROW_A 3     // Digital Pin 3
#define KEYB_PIN_ROW_B 4     // Digital Pin 4
#define KEYB_PIN_ROW_C 5     // Digital Pin 5
#define KEYB_PIN_ROW_D 6     // Digital Pin 6
#define KEYB_PIN_COL_1 7     // Digital Pin 7
#define KEYB_PIN_COL_2 8     // Digital Pin 8
#define KEYB_PIN_COL_3 12    // Digital Pin 12
#define KEYB_PIN_COL_4 13    // Digital Pin 13

// Joystick Uno Pin Definitions (button only)
#define JOY_PIN_SWITCH 2     // Digital Pin 2



// Special Chars
inline byte chrMotorRightA[] = {
    0b11111,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b11111,
    0b00000};

inline byte chrMotorRightB[] = {
    0b11000,
    0b11000,
    0b11000,
    0b11111,
    0b11000,
    0b11000,
    0b11000,
    0b00000};

inline byte chrCircleFilledA[] = {
    0b00111,
    0b01100,
    0b11001,
    0b11011,
    0b11001,
    0b01100,
    0b00111,
    0b00000};

inline byte chrCircleFilledB[] = {
    0b11000,
    0b00100,
    0b10010,
    0b11010,
    0b10010,
    0b00100,
    0b11000,
    0b00000};

inline byte chrMotorLeftA[] = {
    0b00011,
    0b00011,
    0b00011,
    0b11111,
    0b00011,
    0b00011,
    0b00011,
    0b00000};

inline byte chrMotorLeftB[] = {
    0b11111,
    0b00001,
    0b00001,
    0b00001,
    0b00001,
    0b00001,
    0b11111,
    0b00000};

inline byte chrCircleA[] = {
    0b00011,
    0b00110,
    0b01100,
    0b11000,
    0b01100,
    0b00110,
    0b00011,
    0b00000};

inline byte chrCircleB[] = {
    0b11000,
    0b01100,
    0b00110,
    0b00011,
    0b00110,
    0b01100,
    0b11000,
    0b00000};

struct sRowAxis
{
    char buttonKey;
    char buttonKeyUP;
    char buttonKeyDOWN;
    int currentRPM;
    int turnsS;
    int turnsZ;
    char rotation;
};

#endif
