/**
   Maquina de Cuerdas de Guido Wardak
   @author Egar Almeida
*/

#include <Keypad.h>
#include <LiquidCrystal_PCF8574.h>
#include "config.h"
#include "stepperController.h"
#include "utils.h"

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
LiquidCrystal_PCF8574 lcd(0x27); // lcd(0x27);

// Configuration variables

#define BTN_STATES 3
#define BTNS_ROT 4
#define BTNS_SPECIAL 2

bool configState = true;

// Buttons
char rotations[3] = {'s', 'z', 'x'};

sRowAxis rowAxis[BTNS_ROT];

// Motors
StepperController motor[] = {StepperController(rowAxis[ROW_A], MOTOR_PIN_A_STEP, MOTOR_PIN_A_DIR, MOTOR_PIN_A_ENABLED, 32, 200),
                             StepperController(rowAxis[ROW_B], MOTOR_PIN_B_STEP, MOTOR_PIN_B_DIR, MOTOR_PIN_B_ENABLED, 32, 200),
                             StepperController(rowAxis[ROW_C], MOTOR_PIN_C_STEP, MOTOR_PIN_C_DIR, MOTOR_PIN_C_ENABLED, 32, 200),
                             StepperController(rowAxis[ROW_D], MOTOR_PIN_D_STEP, MOTOR_PIN_D_DIR, MOTOR_PIN_D_ENABLED, 32, 200)};

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
  pinMode(MOTOR_PIN_A_ENABLED, OUTPUT);

  // Config display
  displaySetup();

  // Config button actions
  rowAxis[ROW_A].buttonKeyUP = '1';
  rowAxis[ROW_A].buttonKeyDOWN = '2';
  rowAxis[ROW_A].buttonKey = '0';
  rowAxis[ROW_A].currentRPM = 10;
  rowAxis[ROW_A].turnsS = 0;
  rowAxis[ROW_A].turnsZ = 0;
  rowAxis[ROW_A].rotation = 's';

  rowAxis[ROW_B].buttonKeyUP = '5';
  rowAxis[ROW_B].buttonKeyDOWN = '6';
  rowAxis[ROW_B].buttonKey = '4';
  rowAxis[ROW_B].currentRPM = 10;
  rowAxis[ROW_B].turnsS = 0;
  rowAxis[ROW_B].turnsZ = 0;
  rowAxis[ROW_B].rotation = 's';

  rowAxis[ROW_C].buttonKeyUP = '9';
  rowAxis[ROW_C].buttonKeyDOWN = 'A';
  rowAxis[ROW_C].buttonKey = '8';
  rowAxis[ROW_C].currentRPM = 10;
  rowAxis[ROW_C].turnsS = 0;
  rowAxis[ROW_C].turnsZ = 0;
  rowAxis[ROW_C].rotation = 'z';

  rowAxis[ROW_D].buttonKeyUP = 'D';
  rowAxis[ROW_C].buttonKeyDOWN = 'E';
  rowAxis[ROW_C].buttonKey = 'C';
  rowAxis[ROW_C].currentRPM = 10;
  rowAxis[ROW_C].turnsS = 0;
  rowAxis[ROW_C].turnsZ = 0;
  rowAxis[ROW_C].rotation = 'x';

  specialButtonKeys[0] = '3';
  specialButtonKeys[1] = 'F';
}

void loop()
{
  char key = keypad.getKey();

  if (configState)
  {
    digitalWrite(MOTOR_PIN_A_ENABLED, HIGH); // Disable motors
    state_config();
  }
  else
  {
    digitalWrite(MOTOR_PIN_A_ENABLED, LOW); // Enable motors
    state_running();
  }
}

// Machine is on configuration mode
void state_config()
{
  checkJoystick();
  updateDisplay();

  for (byte i = 0; i < 4; i++)
  {
    motor[i].stop();
  }
}

// Machine is running
void state_running()
{
  for (byte i = 0; i < 4; i++)
  {
    if (rowAxis[i].rotation != 'x')
    {

      motor[i].spin(rowAxis[i].currentRPM, rowAxis[i].rotation);
      motor[i].start();
    }
  }

  updateDisplay();
}

// Handle the joystick input
void checkJoystick()
{
  // TODO: set max rpm
  // motor[1].stepFromAxis(analogRead(JOY_PIN_X), 0, 80);
  // motor[3].stepFromAxis(analogRead(JOY_PIN_Y), 0, 80);
  /* TODO
    fuera de Start, mueve B y D con vertical y horizontal respectivamente
    Durante start, lo mismo SALVO que B o D estén en modo S o modo Z (si están en X los podes mover con joystick)
  */
}

void updateDisplay()
{
  // CLS
  lcd.clear();

  // Row icons
  // String rowSymbol[4] = {"\4\5  ", "\2\3  ", "\4\5  ", "\6\7  "};
  String rowSymbol[4] = {"]-",
                          "))",
                          "-[",
                          "<>"};

  // 3 first rows are the same
  for (uint8_t i = 0; i < (BTNS_ROT - 1); i++)
  {
    String rotChar;
    if (rowAxis[i].rotation == 'x')
    {
      rotChar = " ";
    }
    else
    {
      if(i < 3) {
        rotChar = rowAxis[i].rotation;
      }
      else {
        rotChar = rowAxis[i].rotation == 's' ? '<' : '>';   // The last row is not a rotation but a direction
      }
    }

    lcd.setCursor(0, i);
    lcd.print(rowSymbol[i]); // display the row symbol

    lcd.setCursor(4, i);
    lcd.print(String(rotChar)); // display rotation symbol

    lcd.setCursor(rightJus(rowAxis[i].currentRPM, 6), i); // set the cursor for right justification
    lcd.print(rowAxis[i].currentRPM);                     // display rpm

    lcd.setCursor(rightJus(rowAxis[i].turnsS, 11), i);
    lcd.print(rowAxis[i].turnsS); // display s turns

    lcd.setCursor(14, i);
    if(i < 3) {
      lcd.print("s");
    }
    else {
      lcd.print("<");   // The last row is not a rotation but a direction
    }

    lcd.setCursor(rightJus(rowAxis[i].turnsZ, 16), i);
    lcd.print(rowAxis[i].turnsZ); // display z turns

    lcd.setCursor(19, i);
    if(i < 3) {
      lcd.print("z");   // The last row is not a rotation but a direction
    }
    else {
      lcd.print(">");
    }
  }
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
    // buttonPressed(key, true);
    //  TODO
    break;

  case HOLD:
    buttonHeld(key);
    break;
  }
}

/*
  Button Pressed Actions
*/
void buttonPressed(char key, bool released)
{
  // Don't check keys unnecessarily
  if (key != specialButtonKeys[0] && key != specialButtonKeys[1])
  {
    // Act on rotation and speed
    for (int i = 0; i < BTNS_ROT; i++)
    {
      if (key == rowAxis[i].buttonKey)
      {
        cycleButton(i);
        break;
      }
      else if (key == rowAxis[i].buttonKeyUP)
      {

        // Limited to 999 RPM for display safety (3 digits)
        if (rowAxis[i].currentRPM <= 989)
        {
          rowAxis[i].currentRPM += 10;
        }
        else
        {
          rowAxis[i].currentRPM = 999;
        }

        break;
      }
      else if (key == rowAxis[i].buttonKeyDOWN)
      {
        if (rowAxis[i].currentRPM >= 10)
        {
          rowAxis[i].currentRPM -= 10;
        }
        else
        {
          rowAxis[i].currentRPM = 0;
        }

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
      // Copia RPM de A a C e invierte el sentido de giro entre ambos
      if (rowAxis[ROW_A].rotation != 'x')
      {
        // Overwrite C with A
        rowAxis[ROW_C].currentRPM = rowAxis[ROW_A].currentRPM;

        if (rowAxis[ROW_A].rotation == 's')
        {
          rowAxis[ROW_C].rotation = 'z';
        }
        else if (rowAxis[ROW_A].rotation == 'z')
        {
          rowAxis[ROW_C].rotation = 's';
        }
      }
    }
    // Start / Pause
    else if (key == specialButtonKeys[1])
    {

      configState = configState ? false : true;
      Serial.println(configState);
    }
  }
}

/*
  Button Long Pressed Actions
*/
void buttonHeld(char key)
{
  // Don't check keys unnecessarily
  if (key != specialButtonKeys[0] && key != specialButtonKeys[1])
  {
    // Act on rotation and speed buttons
    for (int i = 0; i < BTNS_ROT; i++)
    {
      if (key == rowAxis[i].buttonKey)
      {
        rowReset(i);
        break;
      }

      if (key == rowAxis[i].buttonKeyUP)
      {
        // Limited to 999 RPM for display safety (3 digits)
        if (rowAxis[i].currentRPM <= 998)
        {
          rowAxis[i].currentRPM += 1;
        }
        else
        {
          rowAxis[i].currentRPM = 999;
        }
      }
      else if (key == rowAxis[i].buttonKeyDOWN)
      {
        if (rowAxis[i].currentRPM >= 1)
        {
          rowAxis[i].currentRPM -= 1;
        }
        else
        {
          rowAxis[i].currentRPM = 0;
        }
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
      // COPIA VALORES ENTRE MOTORES (Cada n seg. cambia de pareja de motores (A)-(B) / (B)-(C) / (A)-(C).
      // Titila el dibujo para indicar esta accion.
      // Al soltar, el segundo motor toma los valores del primero.
      // TODO (use switchPair?)
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
  if (rowAxis[i].rotation == rotations[BTN_STATES - 1])
  {
    rowAxis[i].rotation = rotations[0];
  }
  else if (rowAxis[i].rotation == rotations[0])
  {
    rowAxis[i].rotation = rotations[1];
  }
  else if (rowAxis[i].rotation == rotations[1])
  {
    rowAxis[i].rotation = rotations[BTN_STATES - 1];
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
    lcd.createChar(0, chrMotorRightA);
    lcd.createChar(1, chrMotorRightB);
    lcd.createChar(2, chrCircleFilledA);
    lcd.createChar(3, chrCircleFilledB);
    lcd.createChar(4, chrMotorLeftA);
    lcd.createChar(5, chrMotorLeftB);
    lcd.createChar(6, chrCircleA);
    lcd.createChar(7, chrCircleB);
  }
  else
  {
    Serial.println("LCD not found.");
    Serial.print("Error: ");
    Serial.print(error);
  }

  lcd.setBacklight(255);
}
