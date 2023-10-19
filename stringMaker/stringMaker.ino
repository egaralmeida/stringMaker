/**
   Maquina de Cuerdas de Guido Wardak
   @author Egar Almeida
*/
#include <Keypad.h>
#include <LiquidCrystal_PCF8574.h>
#include "config.h"
#include "utils.h"
#include "stepperController.h"

/**
    Keyboard Setup
*/
// How many rows and columns the keyboard has
const byte ROWS = 4;
const byte COLS = 4;

// Keymap
char hexaKeys[ROWS][COLS] = {
    {'0', '1', '2', '3'},
    {'4', '5', '6', '7'},
    {'8', '9', 'A', 'B'},
    {'C', 'D', 'E', 'F'}};

// Pinout
byte rowPins[ROWS] = {KEYB_PIN_ROW_A, KEYB_PIN_ROW_B, KEYB_PIN_ROW_C, KEYB_PIN_ROW_D}; // row pins
byte colPins[COLS] = {KEYB_PIN_COL_1, KEYB_PIN_COL_2, KEYB_PIN_COL_3, KEYB_PIN_COL_4}; // column pins

// Create keypad object with our configuration
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/**
   Display Setup
*/
LiquidCrystal_PCF8574 lcd(0x27); //lcd(0x27);

// Configuration variables

#define BTN_STATES 3
#define BTNS_ROT 4
#define BTNS_SPECIAL 2

bool configState = true;

// Motors
StepperController motorA = StepperController(MOTOR_PIN_A_STEP, MOTOR_PIN_A_DIR, MOTOR_PIN_A_ENABLED, 16, 200);
StepperController motorB = StepperController(MOTOR_PIN_B_STEP, MOTOR_PIN_B_DIR, MOTOR_PIN_B_ENABLED, 16, 200);
StepperController motorC = StepperController(MOTOR_PIN_C_STEP, MOTOR_PIN_C_DIR, MOTOR_PIN_C_ENABLED, 16, 200);
StepperController motorD = StepperController(MOTOR_PIN_D_STEP, MOTOR_PIN_D_DIR, MOTOR_PIN_D_ENABLED, 16, 200);

// Buttons
char buttonStates[3] = {'s', 'z', ' '};

struct sButton
{
  char buttonKey;
  char buttonKeyUP;
  char buttonKeyDOWN;
  int currentRPM;
  int turnsS;
  int turnsZ;
  char buttonState;
};
sButton button[BTNS_ROT];

// Special buttons
char specialButtonKeys[BTNS_SPECIAL];

void setup()
{

#ifdef DEBUG
  Serial.begin(115200);
#endif

  // Config keyboard
  keypad.addEventListener(keypadEvent);

  // Config PINS
  // TODO

  // Config display
  displaySetup();

  // Config button actions
  button[0].buttonKeyUP = '1';
  button[1].buttonKeyUP = '5';
  button[2].buttonKeyUP = '9';
  button[3].buttonKeyUP = 'D';

  button[0].buttonKeyDOWN = '2';
  button[1].buttonKeyDOWN = '6';
  button[2].buttonKeyDOWN = 'A';
  button[3].buttonKeyDOWN = 'E';

  button[0].buttonKey = '0';
  button[1].buttonKey = '4';
  button[2].buttonKey = '8';
  button[3].buttonKey = 'C';

  specialButtonKeys[0] = '3';
  specialButtonKeys[1] = 'F';
}

void loop()
{

  if (configState)
  {
    state_config();
  }
  else
  {
    state_running();
  }
}

// Machine is on configuration mode
void state_config()
{
  checkJoystick();
  updateDisplay();
}

// Machine is running
void state_running()
{
  updateDisplay();
}

// Handle the joystick input
void checkJoystick()
{
  // TODO:  set the correct motors and max rpm
  motorA.stepFromAxis(analogRead(JOY_PIN_X), 0, 80);
  motorB.stepFromAxis(analogRead(JOY_PIN_Y), 0, 80);
}

void updateDisplay()
{
  // CLS
  lcd.clear();

  // Row icons
  String displayRow[4] = {"]- ", "() ", "]- ", "<> "};

  // 3 first rows are the same
  for (int i = 0; i < BTNS_ROT - 1; i++)
  {
    displayRow[i] += button[i].buttonState + " " + rightJustify(button[i].turnsS) + "s " + rightJustify(button[i].turnsZ) + "z";
    debugVarln(displayRow[i]);

    // Place the cursor at the start of each row
    lcd.setCursor(0, i);
    // Print the row
    lcd.print(displayRow[i]);
  }

  // Last row varies
  char arrowDir;
  if (button[3].buttonState == 's')
  {
    arrowDir = '<';
  }
  else if (button[3].buttonState == 'z')
  {
    arrowDir = '>';
  }
  else
  {
    arrowDir = ' ';
  }
  displayRow[3] += arrowDir + " " + rightJustify(button[3].turnsS) + "s " + rightJustify(button[3].turnsZ) + "z";

  debugVarln(displayRow[3]);
  debugln(" ");

  // Place the cursor at the start of row 4
  lcd.setCursor(0, 3);
  // Print the row
  lcd.print(displayRow[3]);
}

// Key event raised
void keypadEvent(KeypadEvent key)
{

  char currKey = key;
  KeyState action = keypad.getState();

  switch (action)
  {
  case PRESSED:
    buttonPressed(key, false);
    break;

  case RELEASED:
    buttonPressed(key, true);
    break;

  case HOLD:
    buttonHeld(key);
    break;
  }
}

void buttonPressed(char key, bool released)
{
  // Don't check keys unnecessarily
  if (key != specialButtonKeys[0] && key != specialButtonKeys[1])
  {
    // Act on rotation and speed
    for (int i = 0; i < BTNS_ROT; i++)
    {
      if (key == button[i].buttonKey)
      {
        cycleButton(i);
        break;
      }
      else if (key == button[i].buttonKeyUP)
      {
        button[i].currentRPM += button[i].currentRPM >= 999 ? 0 : 10; // Limited to 999 RPM for display safety (3 digits)
        break;
      }
      else if (key == button[i].buttonKeyDOWN)
      {
        button[i].currentRPM -= button[i].currentRPM <= 0 ? 0 : 10; // no negative RPM please
        break;
      }
    }
  }
  else
  {
    // Special Buttons
    // Lathe mode
    if (key == specialButtonKeys[0])
    {
      // TODO
    }
    // Start / Pause
    else if (key == specialButtonKeys[1])
    {
      configState = configState ? false : true;
    }
  }
}

void buttonHeld(char key)
{
  // Don't check keys unnecessarily
  if (key != specialButtonKeys[0] && key != specialButtonKeys[1])
  {
    // Act on rotation and speed buttons
    for (int i = 0; i < BTNS_ROT; i++)
    {
      if (key == button[i].buttonKey)
      {
        rowReset(i);
        break;
      }

      if (key == button[i].buttonKeyUP)
      {
        button[i].currentRPM += button[i].currentRPM >= 999 ? 0 : 1; // Limited to 999 RPM for display safety (3 digits)
        break;
      }
      else if (key == button[i].buttonKeyDOWN)
      {
        button[i].currentRPM -= button[i].currentRPM <= 0 ? 0 : 1; // no negative RPM please
        break;
      }
    }
  }
  else
  {
    // Special Buttons
    // Copy values
    if (key == specialButtonKeys[0])
    {
      // TODO
    }
    // General Reset
    else if (key == specialButtonKeys[1])
    {
      // TODO
    }
  }
}

void cycleButton(byte i)
{
  if (button[i].buttonState == buttonStates[BTN_STATES - 1])
  {
    button[i].buttonState = buttonStates[0];
  }
  else if (button[i].buttonState == buttonStates[0])
  {
    button[i].buttonState = buttonStates[1];
  }
  else if (button[i].buttonState == buttonStates[1])
  {
    button[i].buttonState = buttonStates[BTN_STATES - 1];
  }
}

void rowReset(byte row)
{
  // TODO
}

void displaySetup()
{
  int error;
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.println("LCD found.");
    lcd.begin(20, 4);

    // Create custom chars
    // lcd.createChar(1, dotOff);
    // lcd.createChar(2, dotOn);
  }
  else
  {
    Serial.println("LCD not found.");
    Serial.print("Error: ");
    Serial.print(error);
  }

  lcd.setBacklight(255);

}
