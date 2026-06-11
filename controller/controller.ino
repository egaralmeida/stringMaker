#include <Keypad.h>
#include <LiquidCrystal_PCF8574.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "utils.h"

// Communication Pins
#define RX_PIN 10                             // RX for communication (connected to TX on driver Arduino)
#define TX_PIN 11                             // TX for communication (connected to RX on driver Arduino)
#define START_SIGNAL_PIN 9                    // Input pin for start/stop signal from driver
SoftwareSerial driverSerial(RX_PIN, TX_PIN);  // Initialize SoftwareSerial

/**
 * Keyboard Setup
 */
const byte ROWS = 4;
const byte COLS = 4;

// Keymap for keypad
char hexaKeys[ROWS][COLS] = {
  { 'F', 'E', 'D', 'C' },
  { 'B', 'A', '9', '8' },
  { '7', '6', '5', '4' },
  { '3', '2', '1', '0' }
};

// Keypad Pins (defined in config.h)
// Adjust these as needed to match your wiring
byte rowPins[ROWS] = { KEYB_PIN_ROW_A, KEYB_PIN_ROW_B, KEYB_PIN_ROW_C, KEYB_PIN_ROW_D };
byte colPins[COLS] = { KEYB_PIN_COL_1, KEYB_PIN_COL_2, KEYB_PIN_COL_3, KEYB_PIN_COL_4 };

// Keypad object
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/**
 * Display Setup
 */
LiquidCrystal_PCF8574 lcd(0x27);

// Configuration variables
#define BTN_STATES 3
#define BTNS_ROT 4
#define BTNS_SPECIAL 2

bool configState = true;      // Toggle between config and running state
bool releasedAllowed = true;  // Control for button release handling
unsigned long holdTime = 0;   // Time of button hold start
char heldKey = NO_KEY;        // Key that is being held down

// Rotation options for buttons (UPPERCASE to match driver expectations)
char rotations[3] = { 'S', 'Z', 'X' };
sRowAxis rowAxis[BTNS_ROT];  // Holds motor config for each axis

// Special buttons - no longer used for start/stop logic
char specialButtonKeys[BTNS_SPECIAL];

unsigned long lastConfigSentTime = 0;  // Timestamp for the last configuration send
bool wasInConfigMode = true;           // Track previous config state for transitions

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  driverSerial.begin(9600);  // Serial communication with driver Arduino
  
  // Setup start signal pin with internal pullup
  pinMode(START_SIGNAL_PIN, INPUT_PULLUP);

  // Keypad configuration
  keypad.addEventListener(keypadEvent);
  keypad.setHoldTime(750);     // Increase hold time to 750 ms
  keypad.setDebounceTime(20);  // Set debounce time to 20 ms

  // Display setup
  displaySetup();

  // Initialize each motor axis settings
  initializeAxis();
}

void loop() {
  keypad.getKey();  // Continuously check for keypad events

  // Check start signal pin state
  configState = digitalRead(START_SIGNAL_PIN); // HIGH (pulled up) = config mode, LOW = running mode

  // Handle incoming RPM data from driver
  handleIncomingDriverCommands();

  if (configState) {
    wasInConfigMode = true;  // Track that we're in config mode
    state_config();
  } else {
    // Detect transition from config to running mode
    if (wasInConfigMode) {
      wasInConfigMode = false;
      forceResendConfiguration();  // Force immediate config send
    }
    state_running();  // Call state_running when in running mode
  }
}

// Running mode: send configuration to driver with non-blocking delay
void state_running() {
  // Check if 100 ms have passed since the last configuration send
  if (millis() - lastConfigSentTime >= 100) {
    sendMotorConfiguration();       // Call the function to send motor configuration
    lastConfigSentTime = millis();  // Update the last configuration send time
  }

  updateDisplay();  // Refresh the display
}

// Configuration mode: motors are disabled, manual control allowed
void state_config() {
  checkJoystick();
  updateDisplay();
}

// Placeholder for checkJoystick function
void checkJoystick() {
  // Implement joystick control as desired
}

// Static variable for command caching (declared outside functions for access by forceResend)
static String lastSentCommand = "";

// Force re-send motor configuration (called on transition to running mode)
void forceResendConfiguration() {
  lastSentCommand = "";  // Clear cache to force re-send
  sendMotorConfiguration();  // Send immediately
#ifdef DEBUG
  Serial.println("Forced resend of motor configuration");
#endif
}

// Send motor settings via serial only if different from previous command
void sendMotorConfiguration() {
  String command = "M";            // Start command with 'M' to indicate motor data

  // Construct configuration command for each motor
  for (byte i = 0; i < BTNS_ROT; i++) {
    char direction = rowAxis[i].rotation;  // Get the rotation character ('S', 'Z', 'X')
    int rpm = rowAxis[i].currentRPM;       // Get the RPM for the current motor

    // Cap RPM to 999 for compatibility with driver Arduino
    if (rpm > 999)
      rpm = 999;

    // Append motor direction and RPM in 4-character format
    command += direction;  // Direction: 'S', 'Z', 'X'
    if (rpm < 100) command += "0";
    if (rpm < 10) command += "0";
    command += String(rpm);  // RPM in zero-padded 3-digit format
  }

  command += "Q";  // End command with 'Q' as termination character

  // Only send if command is different from the last sent command
  if (command != lastSentCommand) {
    driverSerial.println(command);  // Send new command
    lastSentCommand = command;      // Update the last command

#ifdef DEBUG
    Serial.println("Sent to driver: " + command);
#endif
  }
}

// Initialize axis settings for each motor
void initializeAxis() {
  rowAxis[ROW_A] = { '0', '1', '2', 20, 0, 0, 'S' };
  rowAxis[ROW_B] = { '4', '5', '6', 20, 0, 0, 'S' };
  rowAxis[ROW_C] = { '8', '9', 'A', 20, 0, 0, 'Z' };
  rowAxis[ROW_D] = { 'C', 'D', 'E', 20, 0, 0, 'X' };

  specialButtonKeys[0] = '3';
  specialButtonKeys[1] = 'F';
}

// Handle incoming driver commands - now only RPM data
void handleIncomingDriverCommands() {
  while (driverSerial.available() > 0) {
    String incoming = driverSerial.readStringUntil('\n');
    incoming.trim();

    if (incoming.startsWith("R") && incoming.endsWith("Q")) {
      // RPM report handling
      parseRPMData(incoming);
    } else if (incoming.startsWith("M") && incoming.endsWith("Q")) {
      // Handle motor configuration from driver if needed
      // Typically, the controller sends M...Q commands, not receives them.
      // But if needed, implement parsing here.
    }
  }
}

// Parse the RPM data from the driver
void parseRPMData(String data) {
  // Data format: R<ZZZZZ><SSSSS>...Q
  // For each motor, we have 5 digits for Z count and 5 digits for S count
  for (int i = 0; i < BTNS_ROT; i++) {
    int zIndex = 1 + (i * 10);  // Calculate index for Z count in the data string
    int sIndex = zIndex + 5;    // Calculate index for S count in the data string

    // Parse the Z and S turn counts for each motor (5 characters each)
    rowAxis[i].turnsZ = data.substring(zIndex, zIndex + 5).toInt();
    rowAxis[i].turnsS = data.substring(sIndex, sIndex + 5).toInt();
  }

#ifdef DEBUG
  Serial.println("Updated RPM counts from driver: " + data);
#endif
}

// Update the LCD display with current motor states
void updateDisplay() {
  byte rowCharA[] = { 0, 2, 4, 6 };  // chrMotorRightA, chrCircleFilledA, chrMotorLeftA, chrCircleA
  byte rowCharB[] = { 1, 3, 5, 7 };  // chrMotorRightB, chrCircleFilledB, chrMotorLeftB, chrCircleB

  for (uint8_t i = 0; i < BTNS_ROT; i++) {
    lcd.setCursor(0, i);
    lcd.write(rowCharA[i]);
    lcd.setCursor(1, i);
    lcd.write(rowCharB[i]);

    String rotChar = rowAxis[i].rotation == 'X' ? " " : String(rowAxis[i].rotation);
    lcd.setCursor(4, i);
    lcd.print(rotChar);

    // Print fixed-width, space-padded fields so shrinking values
    // don't leave stale digits on screen
    lcd.setCursor(6, i);
    lcd.print(rightJustify(rowAxis[i].currentRPM, 3));

    lcd.setCursor(10, i);
    lcd.print(rightJustify((int)min(rowAxis[i].turnsS, 9999L), 4));

    lcd.setCursor(14, i);
    lcd.print(i < 3 ? "s" : "<");

    lcd.setCursor(15, i);
    lcd.print(rightJustify((int)min(rowAxis[i].turnsZ, 9999L), 4));

    lcd.setCursor(19, i);
    lcd.print(i < 3 ? "z" : ">");
  }
}

// Keypad event handler, modified to no longer handle start/stop from keypad
void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {
    case PRESSED:
      if (millis() - holdTime > 20) {  // Debounce
        buttonPressed(key, false);
        heldKey = key;
        holdTime = millis();
        releasedAllowed = true;
      }
      break;

    case HOLD:
      if (heldKey == key) {
        buttonHeld(key);
      }
      break;

    case RELEASED:
      if (heldKey == key && releasedAllowed) {
        buttonPressed(heldKey, true);
        heldKey = NO_KEY;
      }
      break;
  }
}

// Handle button presses and releases (no longer toggling start/stop)
void buttonPressed(char key, bool released) {
  // Special keys no longer handle start/stop logic
  if (key == specialButtonKeys[0]) {
    if (!released) {
      // Example: Copy RPM from A to C and invert direction (as per original requirements)
      rowAxis[ROW_C].currentRPM = rowAxis[ROW_A].currentRPM;
      rowAxis[ROW_C].rotation = rowAxis[ROW_A].rotation == 'S' ? 'Z' : 'S';
    }
  } else if (key == specialButtonKeys[1]) {
    // Previously toggled configState here, now disabled
    // No action
  } else {
    // Handle rotation and RPM adjustments
    for (int i = 0; i < BTNS_ROT; i++) {
      if (key == rowAxis[i].buttonKey && !released) {
        cycleButton(i);
        break;
      } else if (key == rowAxis[i].buttonKeyUP && released) {
        rowAxis[i].currentRPM = min(rowAxis[i].currentRPM + 10, 999);
        break;
      } else if (key == rowAxis[i].buttonKeyDOWN && released) {
        rowAxis[i].currentRPM = max(rowAxis[i].currentRPM - 10, 0);
        break;
      }
    }
  }
  lcd.clear();  // Refresh display if needed
}

// Handle long button press
void buttonHeld(char key) {
  releasedAllowed = false;
  for (int i = 0; i < BTNS_ROT; i++) {
    if (key == rowAxis[i].buttonKey) {
      rowReset(i);
      break;
    } else if (key == rowAxis[i].buttonKeyUP) {
      rowAxis[i].currentRPM = min(rowAxis[i].currentRPM + 1, 999);
      break;
    } else if (key == rowAxis[i].buttonKeyDOWN) {
      rowAxis[i].currentRPM = max(rowAxis[i].currentRPM - 1, 0);
      break;
    }
  }
  lcd.clear();
}

// Cycle motor rotation state
void cycleButton(byte i) {
  // rotations: {'S','Z','X'}
  // Cycles through S->Z->X->S...
  rowAxis[i].rotation = rowAxis[i].rotation == rotations[BTN_STATES - 1] ? rotations[0] :
                        rowAxis[i].rotation == rotations[0] ? rotations[1] : rotations[BTN_STATES - 1];
}

// Reset specific motor row
void rowReset(byte row) {
  rowAxis[row].currentRPM = 0;
  rowAxis[row].turnsS = 0;
  rowAxis[row].turnsZ = 0;
  rowAxis[row].rotation = 'X';
}

// Initialize display and custom characters
void displaySetup() {
  Wire.begin();
  Wire.beginTransmission(0x27);
  int error = Wire.endTransmission();

  if (error == 0) {
    lcd.begin(20, 4);
    lcd.createChar(0, chrMotorRightA);
    lcd.createChar(1, chrMotorRightB);
    lcd.createChar(2, chrCircleFilledA);
    lcd.createChar(3, chrCircleFilledB);
    lcd.createChar(4, chrMotorLeftA);
    lcd.createChar(5, chrMotorLeftB);
    lcd.createChar(6, chrCircleA);
    lcd.createChar(7, chrCircleB);
    lcd.setBacklight(255);

#ifdef DEBUG
    Serial.println("LCD initialized successfully.");
#endif
  } else {
#ifdef DEBUG
    Serial.println("Error: LCD not found.");
    Serial.print("Error code: ");
    Serial.println(error);
#endif
  }
}
