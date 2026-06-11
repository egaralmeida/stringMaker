// This sketch needs a second hardware serial port to talk to the controller.
// If this fires, the compile is not actually targeting a Mega 2560 - check
// Tools > Board (and that no custom/old AVR core is shadowing the official one).
#if !defined(HAVE_HWSERIAL1)
#error "Board has no Serial1 - select Tools > Board > Arduino Mega or Mega 2560"
#endif

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
  {MOTOR_PIN_A_STEP, MOTOR_PIN_A_DIR, nullptr, 0, nullptr, 0, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_B_STEP, MOTOR_PIN_B_DIR, nullptr, 0, nullptr, 0, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_C_STEP, MOTOR_PIN_C_DIR, nullptr, 0, nullptr, 0, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_D_STEP, MOTOR_PIN_D_DIR, nullptr, 0, nullptr, 0, 400, true, false, 0, 750, 0, 0}
};

String incomingData = ""; // Buffer for incoming serial data
const char START_CHAR = 'M'; // Start character for config command
const char END_CHAR = 'Q';   // End character for config command

bool systemActive = false;    // True if system is running
unsigned long lastReportTime = 0;    // Timer for reporting RPM counts every 500 ms
unsigned long lastButtonPress = 0;   // Debounce timer for button
bool lastButtonState = HIGH;  // Track previous button state for edge detection

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
    if (incomingData.length() == 0 && incomingChar != START_CHAR) {
      continue;  // Skip garbage until we find 'M'
    }
    
    incomingData += incomingChar;  // Accumulate valid characters

    // Process when a complete 18-character command is received
    if (incomingData.length() == 18 && incomingData.charAt(0) == START_CHAR && incomingData.charAt(17) == END_CHAR) {
      Serial.print("Received raw data: ");
      Serial.println(incomingData);

      // Parse and process the 18-character command
      processIncomingData(incomingData);

      // Clear incomingData
      incomingData = "";
    }
    // Clear buffer if command is malformed or incomplete
    else if (incomingData.length() >= 18) {
      Serial.println("Error: Incomplete or malformed command.");
      incomingData = "";
    }
  }
}

// Parse and process M...Q command for each motor
void processIncomingData(String data) {
  for (int i = 0; i < 4; i++) {
    int index = 1 + (i * 4); 
    char direction = data.charAt(index); // Direction (S, Z, or X)
    int rpm = data.substring(index + 1, index + 4).toInt();

    if (direction == 'X' || rpm <= 0) {
      // 'X' or 0 RPM both mean "do not move" (also guards the interval division below)
      motors[i].active = false;
      motors[i].rpm = 0;
    } else {
      motors[i].rpm = rpm;
      motors[i].dir = (direction == 'Z'); // 'Z' for CW (true), 'S' for CCW (false)

      // Calculate step interval; force 32-bit math (200 * rpm overflows 16-bit int above 163 RPM)
      motors[i].interval = 60000000UL / ((unsigned long)STEPS_PER_REV * (unsigned long)rpm);

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

    // Debug
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

// Handle start/stop button with edge detection (only trigger once per press)
void handleButton() {
  bool currentButtonState = digitalRead(BTN_PIN);
  
  // Only trigger on falling edge (HIGH -> LOW transition, i.e., button press)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    // Debounce - increased to 200ms to prevent erratic behavior
    if (millis() - lastButtonPress > 200) {
      lastButtonPress = millis();
      
      // Toggle system state
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
  }
  
  lastButtonState = currentButtonState;  // Update state for next iteration
}

// Report cumulative turn counts every 500ms when running and once before stopping.
// Counts accumulate for the whole run (reset on system start), so the controller
// always displays the total number of turns wound so far.
void reportRPMCounts() {
  String report = "R"; // Start with R
  char buffer[6];

  for (int i = 0; i < 4; i++) {
    // Z direction: convert steps to whole turns, keep within 5 digits
    sprintf(buffer, "%05lu", (motors[i].stepCountZ / STEPS_PER_REV) % 100000UL);
    report += buffer;

    // S direction
    sprintf(buffer, "%05lu", (motors[i].stepCountS / STEPS_PER_REV) % 100000UL);
    report += buffer;
  }

  report += "Q"; // End with Q
  Serial1.println(report); // Send to controller

#ifdef DEBUG
  Serial.println("Sent RPM report: " + report);
#endif
}
