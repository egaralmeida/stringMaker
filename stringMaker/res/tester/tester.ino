#define X_STEP_PIN         12
#define X_DIR_PIN          13
#define X_ENABLE_PIN       8

// RPMs del motor
// nema 17
// polea chica
// con 4x microstepping 140 perfecto, 150 al limite
// con 1x microstepping 130 perfecto, 140 al limite

//polea grande
// con 4x microstepping 30 perfecto, 50 limite
// con 1x microstepping 30 perfecto, 40 limite, arranca hasta 60

// nema 23
// polea grande
// con 4x microstepping 230 perfecto
// con 1x microstepping 220 perfecto, 230 usable, 240 limite


void setup() {

  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(X_ENABLE_PIN, OUTPUT);

  digitalWrite(X_ENABLE_PIN, LOW);
  digitalWrite(X_DIR_PIN, HIGH);
}

unsigned long lastTime = 0;

int RPMX = 400 * 4;

unsigned int RPM_X = 300000 / RPMX;


void loop() {

  doStep (X_STEP_PIN, micros() % RPM_X < 100);

}


void doStep(int pin, bool state) {
  if (state)
  {
    digitalWrite(pin, HIGH);
  }
  else
  {
    digitalWrite(pin, LOW);
  }
}
