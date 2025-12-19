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

// Define structure to hold motor settings
struct Motor {
  int stepPin;
  int dirPin;
  int rpm;         // Speed in RPM
  bool dir;        // true for clockwise (Z), false for counterclockwise (S)
  bool active;     // Motor active status
  unsigned long lastStepTime; // Last step timestamp for non-blocking control
  unsigned int interval;      // Step interval in microseconds
  unsigned long turnCountZ;   // Rotation count in Z direction
  unsigned long turnCountS;   // Rotation count in S direction
};

// Create motor instances
Motor motors[4] = {
  {MOTOR_PIN_A_STEP, MOTOR_PIN_A_DIR, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_B_STEP, MOTOR_PIN_B_DIR, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_C_STEP, MOTOR_PIN_C_DIR, 400, true, false, 0, 750, 0, 0},
  {MOTOR_PIN_D_STEP, MOTOR_PIN_D_DIR, 400, true, false, 0, 750, 0, 0}
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

  // Set up pins for each motor
  for (int i = 0; i < 4; i++) {
    pinMode(motors[i].stepPin, OUTPUT);
    pinMode(motors[i].dirPin, OUTPUT);
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

    if (direction == 'X') {
      motors[i].active = false;
      motors[i].rpm = 0;
    } else {
      motors[i].rpm = rpm;
      motors[i].dir = (direction == 'Z'); // 'Z' for CW (true), 'S' for CCW (false)
      motors[i].interval = 300000 / (rpm * 4); // Convert RPM to interval in µs
      motors[i].active = true;
      digitalWrite(motors[i].dirPin, motors[i].dir ? HIGH : LOW);
    }

    // Debug
    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.print(" - Direction: ");
    Serial.print(motors[i].dir ? "Clockwise (Z)" : "Counterclockwise (S)");
    Serial.print(", RPM: ");
    Serial.print(rpm);
    Serial.print(", Active: ");
    Serial.println(motors[i].active ? "Yes" : "No");
  }
}

// Non-blocking stepping
void doStep(Motor &motor, unsigned long currentTime) {
  if (motor.active && (currentTime - motor.lastStepTime >= motor.interval)) {
    digitalWrite(motor.stepPin, HIGH);
    delayMicroseconds(10); 
    digitalWrite(motor.stepPin, LOW);
    motor.lastStepTime = currentTime;

    // Increment rotation count based on direction
    if (motor.dir) {
      motor.turnCountZ++;
    } else {
      motor.turnCountS++;
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
        // System starting
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

// Report RPM counts every 500ms when running and once before stopping
void reportRPMCounts() {
  String report = "R"; // Start with R
  char buffer[6];

  for (int i = 0; i < 4; i++) {
    // Z direction
    sprintf(buffer, "%05lu", motors[i].turnCountZ);
    report += buffer;

    // S direction
    sprintf(buffer, "%05lu", motors[i].turnCountS);
    report += buffer;
  }

  report += "Q"; // End with Q
  Serial1.println(report); // Send to controller

  // Reset counts after sending
  for (int i = 0; i < 4; i++) {
    motors[i].turnCountZ = 0;
    motors[i].turnCountS = 0;
  }

#ifdef DEBUG
  Serial.println("Sent RPM report: " + report);
#endif
}
