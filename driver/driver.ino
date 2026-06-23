// This sketch needs a second hardware serial port to talk to the controller.
// If this fires, the compile is not actually targeting a Mega 2560 - check
// Tools > Board (and that no custom/old AVR core is shadowing the official one).
#if !defined(HAVE_HWSERIAL1)
#error "Board has no Serial1 - select Tools > Board > Arduino Mega or Mega 2560"
#endif

// Uncomment for verbose serial diagnostics. Note: the debug prints can block
// the step loop for >100ms at 9600 baud, pausing all motors - keep disabled
// during production winding.
//#define DEBUG

// Pin assignments for each motor
#define MOTOR_PIN_A_STEP 2
#define MOTOR_PIN_A_DIR 5

#define MOTOR_PIN_B_ENABLED 8  // Shared enable pin for all motors
#define MOTOR_PIN_B_STEP 3
#define MOTOR_PIN_B_DIR 6

#define MOTOR_PIN_C_STEP 4
#define MOTOR_PIN_C_DIR 7

#define MOTOR_PIN_D_STEP 12
#define MOTOR_PIN_D_DIR 13

// Button, LED and Signal Pins
#define BTN_PIN 38
#define LED_PIN 40
#define START_SIGNAL_PIN 24  // Output pin to signal controller

// Joystick Mega Pin Definitions (not used in this snippet, but defined for completeness)
#define JOY_MEGA_PIN_X A8
#define JOY_MEGA_PIN_Y A9

// Centralized Constants
const int STEPS_PER_REV = 200;
const unsigned long MIN_STEP_INTERVAL_US = 500;
const unsigned int STEP_PULSE_WIDTH_US = 5;

// Output-shaft steps per revolution for each motor (A, B, C, D), accounting
// for gearing. Motor B drives its output through a 1:3 gearbox, so the motor
// itself must turn 3 motor-revolutions for every 1 output-shaft revolution -
// commanded/displayed RPM and turn counts are all in output-shaft terms.
const unsigned long STEPS_PER_OUTPUT_REV[4] = {
  STEPS_PER_REV,      // A: 1:1
  STEPS_PER_REV * 3,  // B: 1:3
  STEPS_PER_REV,      // C: 1:1
  STEPS_PER_REV       // D: 1:1
};

// Helper macros for direct port manipulation
#define STEP_HIGH(m) (*((m).stepPort) |=  (m).stepMask)
#define STEP_LOW(m)  (*((m).stepPort) &= ~(m).stepMask)
#define DIR_HIGH(m)  (*((m).dirPort)  |=  (m).dirMask)
#define DIR_LOW(m)   (*((m).dirPort)  &= ~(m).dirMask)

// Define structure to hold motor settings
struct Motor {
  int stepPin;
  int dirPin;
  volatile uint8_t *stepPort;
  uint8_t stepMask;
  volatile uint8_t *dirPort;
  uint8_t dirMask;
  int rpm;                    // Speed in RPM
  bool dir;                   // true for clockwise (Z), false for counterclockwise (S)
  bool active;                // Motor active status
  unsigned long lastStepTime; // Last step timestamp for non-blocking control
  unsigned long interval;     // Step interval in microseconds
  unsigned long stepCountZ;   // Step count in Z direction (200 steps = 1 turn)
  unsigned long stepCountS;   // Step count in S direction (200 steps = 1 turn)
};

// Create motor instances (port pointers initialized in setup())
Motor motors[4] = {
  {MOTOR_PIN_A_STEP, MOTOR_PIN_A_DIR, nullptr, 0, nullptr, 0, 50, true, false, 0, 6000, 0, 0},
  {MOTOR_PIN_B_STEP, MOTOR_PIN_B_DIR, nullptr, 0, nullptr, 0, 50, true, false, 0, 2000, 0, 0},
  {MOTOR_PIN_C_STEP, MOTOR_PIN_C_DIR, nullptr, 0, nullptr, 0, 50, true, false, 0, 6000, 0, 0},
  {MOTOR_PIN_D_STEP, MOTOR_PIN_D_DIR, nullptr, 0, nullptr, 0, 50, true, false, 0, 6000, 0, 0}
};

const char START_CHAR = 'M'; // Start character for config command
const char END_CHAR = 'Q';   // End character for config command
const int CMD_LENGTH = 18;   // Full M...Q command length

// Fixed-size receive buffer - avoids String's per-character heap reallocation,
// which was stalling the step loop (and the periodic RPM report below) long
// enough to cause an audible micro-halt across all motors
char incomingData[CMD_LENGTH];
int incomingLength = 0;

bool systemActive = false;    // True if system is running
unsigned long lastReportTime = 0;    // Timer for reporting RPM counts every 500 ms

// Button glitch filter: the pin must hold a new state continuously for
// BTN_STABLE_MS before it is accepted. A single noisy read (EMI coupled
// into the button wiring from the stepper cables) can no longer stop the
// system mid-run.
const unsigned long BTN_STABLE_MS = 50;
bool lastButtonReading = HIGH;       // Last raw pin reading
unsigned long lastButtonChange = 0;  // When the raw reading last changed
// Starts true so a button already reading LOW at boot (e.g. a maintained
// switch left closed from the last session) cannot auto-start the system -
// a genuine release (stable HIGH) is required before any press can register
bool buttonPressHandled = true;

void setup() {
  Serial.begin(9600);      // Debugging over the main serial port
  Serial1.begin(9600);     // Use Serial1 for communication with the Arduino Uno (controller)

  // Set up pins for each motor and derive the port registers / bit masks
  // for direct port manipulation from the pin numbers, so the mapping
  // always matches the board the sketch is compiled for
  for (int i = 0; i < 4; i++) {
    pinMode(motors[i].stepPin, OUTPUT);
    pinMode(motors[i].dirPin, OUTPUT);

    motors[i].stepPort = portOutputRegister(digitalPinToPort(motors[i].stepPin));
    motors[i].stepMask = digitalPinToBitMask(motors[i].stepPin);
    motors[i].dirPort  = portOutputRegister(digitalPinToPort(motors[i].dirPin));
    motors[i].dirMask  = digitalPinToBitMask(motors[i].dirPin);
  }

  // Set up shared enable pin, button, LED and start signal
  pinMode(MOTOR_PIN_B_ENABLED, OUTPUT);
  digitalWrite(MOTOR_PIN_B_ENABLED, HIGH); // Disable all motors initially
  pinMode(BTN_PIN, INPUT_PULLUP);  // Start/Stop button
  pinMode(LED_PIN, OUTPUT);        // Status LED
  digitalWrite(LED_PIN, LOW);      // LED off initially
  pinMode(START_SIGNAL_PIN, OUTPUT);
  digitalWrite(START_SIGNAL_PIN, HIGH);  // HIGH = stopped initially
}

void loop() {
  handleButton();
  unsigned long currentTime = micros();
  bool anyMotorActive = false;

  if (systemActive) {
  
    // Run motors
    for (int i = 0; i < 4; i++) {
      if (motors[i].active) {
        doStep(motors[i], currentTime);
        anyMotorActive = true;
      }
    }

    // Report RPM counts every 500ms
    if (millis() - lastReportTime >= 500) {
      reportRPMCounts();
      lastReportTime = millis();
    }
  }

  // Update LED based on motor activity
  digitalWrite(LED_PIN, (systemActive || anyMotorActive) ? HIGH : LOW);

  // Handle incoming commands from controller
  handleIncomingData();
}

// Handle incoming M...Q configuration commands
void handleIncomingData() {
  while (Serial1.available() > 0) {
    char incomingChar = Serial1.read();
    
    // Skip CR/LF characters (from println)
    if (incomingChar == '\r' || incomingChar == '\n') {
      continue;
    }
    
    // If buffer is empty, only accept 'M' as start (sync to command start)
    if (incomingLength == 0 && incomingChar != START_CHAR) {
      continue;  // Skip garbage until we find 'M'
    }

    incomingData[incomingLength++] = incomingChar;  // Accumulate valid characters

    // Process when a complete 18-character command is received
    if (incomingLength == CMD_LENGTH && incomingData[0] == START_CHAR && incomingData[CMD_LENGTH - 1] == END_CHAR) {
#ifdef DEBUG
      Serial.print("Received raw data: ");
      Serial.write(incomingData, CMD_LENGTH);
      Serial.println();
#endif

      // Parse and process the 18-character command
      processIncomingData(incomingData);

      // Clear incomingData
      incomingLength = 0;
    }
    // Clear buffer if command is malformed or incomplete
    else if (incomingLength >= CMD_LENGTH) {
#ifdef DEBUG
      Serial.println("Error: Incomplete or malformed command.");
#endif
      incomingLength = 0;
    }
  }
}

// Parses exactly 3 ASCII digit characters (the protocol's zero-padded RPM field)
int parseRpm3(const char *digits) {
  return (digits[0] - '0') * 100 + (digits[1] - '0') * 10 + (digits[2] - '0');
}

// Parse and process M...Q command for each motor
void processIncomingData(const char *data) {
  for (int i = 0; i < 4; i++) {
    int index = 1 + (i * 4);
    char direction = data[index]; // Direction (S, Z, or X)
    int rpm = parseRpm3(&data[index + 1]);

    if (direction == 'X' || rpm <= 0) {
      // 'X' or 0 RPM both mean "do not move" (also guards the interval division below)
      motors[i].active = false;
      motors[i].rpm = 0;
    } else {
      motors[i].rpm = rpm;
      motors[i].dir = (direction == 'Z'); // 'Z' for CW (true), 'S' for CCW (false)

      // Calculate step interval from the desired output-shaft RPM; force 32-bit
      // math (200 * rpm overflows 16-bit int above 163 RPM)
      motors[i].interval = 60000000UL / (STEPS_PER_OUTPUT_REV[i] * (unsigned long)rpm);

      // Clamp to minimum step interval
      if (motors[i].interval < MIN_STEP_INTERVAL_US) {
        motors[i].interval = MIN_STEP_INTERVAL_US;
      }

      // Restart step scheduling when (re)activating so the motor doesn't burst to catch up
      if (!motors[i].active) {
        motors[i].lastStepTime = micros();
      }
      motors[i].active = true;

      // Set direction with timing delay using direct port manipulation
      if (motors[i].dir) {
        DIR_HIGH(motors[i]);
      } else {
        DIR_LOW(motors[i]);
      }
      delayMicroseconds(STEP_PULSE_WIDTH_US);
    }

#ifdef DEBUG
    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.print(" - Direction: ");
    Serial.print(motors[i].dir ? "Clockwise (Z)" : "Counterclockwise (S)");
    Serial.print(", RPM: ");
    Serial.print(rpm);
    Serial.print(", Interval: ");
    Serial.print(motors[i].interval);
    Serial.print("us, Active: ");
    Serial.println(motors[i].active ? "Yes" : "No");
#endif
  }
}

// Non-blocking stepping with direct port manipulation
void doStep(Motor &motor, unsigned long currentTime) {
  if (motor.active && (currentTime - motor.lastStepTime >= motor.interval)) {
    STEP_HIGH(motor);
    delayMicroseconds(STEP_PULSE_WIDTH_US);
    STEP_LOW(motor);

    // Schedule from the previous deadline (not "now") so loop latency doesn't
    // accumulate and lower the effective RPM
    motor.lastStepTime += motor.interval;

    // If we've fallen more than one full interval behind, resync instead of
    // bursting steps to catch up
    if (currentTime - motor.lastStepTime >= motor.interval) {
      motor.lastStepTime = currentTime;
    }

    // Increment step count based on direction
    if (motor.dir) {
      motor.stepCountZ++;
    } else {
      motor.stepCountS++;
    }
  }
}

// Handle start/stop button with a stability (glitch) filter: the pin must
// read LOW continuously for BTN_STABLE_MS before a press is accepted, and
// must return to a stable HIGH before the next press can trigger.
void handleButton() {
  bool reading = digitalRead(BTN_PIN);

  // Raw reading changed: restart the stability timer
  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChange = millis();
    return;
  }

  // Reading has been stable long enough to trust
  if (millis() - lastButtonChange >= BTN_STABLE_MS) {
    if (reading == LOW && !buttonPressHandled) {
      buttonPressHandled = true;  // Act once per press
      toggleSystem();
    } else if (reading == HIGH) {
      buttonPressHandled = false;  // Re-arm after a stable release
    }
  }
}

// Toggle between running and stopped states
void toggleSystem() {
  systemActive = !systemActive;

  // Update LED immediately with state change
  digitalWrite(LED_PIN, systemActive ? HIGH : LOW);

  if (systemActive) {
    // System starting - reset counters so each run counts turns from zero
    for (int i = 0; i < 4; i++) {
      motors[i].stepCountZ = 0;
      motors[i].stepCountS = 0;
    }
    digitalWrite(MOTOR_PIN_B_ENABLED, LOW);  // Enable motors
    digitalWrite(START_SIGNAL_PIN, LOW);     // Signal controller we're running
    Serial.println("System STARTED - Motors enabled, waiting for config...");
  } else {
    // System stopping
    digitalWrite(MOTOR_PIN_B_ENABLED, HIGH); // Disable motors
    digitalWrite(START_SIGNAL_PIN, HIGH);    // Signal controller we're stopped
    reportRPMCounts();                      // Send final counts
    // Deactivate all motors
    for (int i = 0; i < 4; i++) {
      motors[i].active = false;
    }
    Serial.println("System STOPPED - Motors disabled");
  }
}

// Report cumulative turn counts every 500ms when running and once before stopping.
// Counts accumulate for the whole run (reset on system start), so the controller
// always displays the total number of turns wound so far.
void reportRPMCounts() {
  // "R" + 4 motors * (5 Z digits + 5 S digits) + "Q" + '\0'
  char report[44];
  report[0] = 'R';
  int pos = 1;

  for (int i = 0; i < 4; i++) {
    // Z direction: convert raw motor steps to whole output-shaft turns, keep within 5 digits
    sprintf(&report[pos], "%05lu", (motors[i].stepCountZ / STEPS_PER_OUTPUT_REV[i]) % 100000UL);
    pos += 5;

    // S direction
    sprintf(&report[pos], "%05lu", (motors[i].stepCountS / STEPS_PER_OUTPUT_REV[i]) % 100000UL);
    pos += 5;
  }

  report[pos++] = 'Q'; // End with Q
  report[pos] = '\0';

  Serial1.println(report); // Send to controller

#ifdef DEBUG
  Serial.print("Sent RPM report: ");
  Serial.println(report);
#endif
}
