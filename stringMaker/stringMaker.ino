/**
   Maquina de Cuerdas de Guido Wardak
   @author Egar Almeida
*/
#include <Keypad.h>
#define DEBUG

#ifdef DEBUG
#define debugln(x) Serial.println(F(x))
#define debug(x) Serial.print(F(x))

#else
#define debugln(x)
#define debug(x)

#endif
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
byte rowPins[ROWS] = {3, 2, 1, 0}; // row pins
byte colPins[COLS] = {7, 6, 5, 4}; // column pins

// Create keypad object with our configuration
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/**
   Display Setup
*/

// Program configuration variables

#define BTN_STATES 3
#define BTNS_ROT 4
#define BTNS_SPECIAL 2

bool configState = true;

char buttonStates[3] = {'s', 'z', ' '};

// Buttons
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

  // Config display
  // TODO

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
  // TODO
}

void updateDisplay()
{
  // TODO

  // Row icons
  String displayRow[4] = {"]- ", "() ", "]- ", "<> "};

  // 3 first rows are the same
  for (int i = 0; i < BTNS_ROT - 1; i++)
  {
    displayRow[i] += button[i].buttonState + " " + String(button[i].turnsS) + "s " + String(button[i].turnsZ) + "z";
#ifdef DEBUG
    Serial.println(displayRow[i]);
#endif
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
  displayRow[3] += arrowDir + " " + String(button[3].turnsS) + "s " + String(button[3].turnsZ) + "z";
#ifdef DEBUG
  Serial.println(displayRow[3]);
  Serial.println("---------------------------------------------");
#endif
}

// Key event raised
void keypadEvent(KeypadEvent key)
{

  char currKey = key;
  KeyState action = keypad.getState();

  actOnKeyEvent(key, action);
}

// Handle key events according to machine state
void actOnKeyEvent(char key, KeyState action)
{

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
      button[i].currentRPM += 10;
      break;
    }
    else if (key == button[i].buttonKeyDOWN)
    {
      button[i].currentRPM -= 10;
      break;
    }
  }

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

void buttonHeld(char key)
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
      button[i].currentRPM += 1;
      break;
    }
    else if (key == button[i].buttonKeyDOWN)
    {
      button[i].currentRPM -= 1;
      break;
    }
  }

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
