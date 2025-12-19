#include <SoftwareSerial.h>

#define RX_PIN 10  // Define RX pin for SoftwareSerial (connect to Mega TX1 - pin 18)
#define TX_PIN 11  // Define TX pin for SoftwareSerial (connect to Mega RX1 - pin 19)

SoftwareSerial megaSerial(RX_PIN, TX_PIN);  // Initialize SoftwareSerial for communication with Mega

bool toggleDirection = false; // Variable to alternate command configurations

void setup() {
  Serial.begin(9600);      // For debugging on Uno's main Serial
  megaSerial.begin(9600);  // For communication with Mega
}

void loop() {
  // Construct the full 16-character command for all motors
  String command;
  if (toggleDirection) {
    // Configuration 1: Motors A and B clockwise, C and D counterclockwise
    command = "MS100Z100S150X000Q";
  } else {
    // Configuration 2: Motors A and B counterclockwise, C stopped, D clockwise
    command = "MX150S180S200Z200Q";
  }
  toggleDirection = !toggleDirection; // Toggle for next loop iteration

  // Send the 16-character command to the Mega
  megaSerial.print(command); // Send command directly
  Serial.println("Sent to Mega: " + command);  // Debugging on Uno Serial
=
  delay(2000);  
  
  if (megaSerial.available() > 0) {
    String incomingData = megaSerial.readStringUntil('\n');
    Serial.println("From Mega: " + incomingData);
  }
}
