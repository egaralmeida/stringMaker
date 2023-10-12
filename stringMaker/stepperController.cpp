#include <Arduino.h>
#include "stepperController.h"

StepperController::StepperController(int stepPin, int dirPin, int enablePin, int microsteps, int steps)
{
    this->stepPin = stepPin;
    this->dirPin = dirPin;
    this->enablePin = enablePin;
    this->microsteps = microsteps;
    this->steps = steps;
    this->running = false;
    this->rpm = 0;

    this->resolution = (float)360 / (float)(steps * this->microsteps); // resolution is constant after this

    // Initialize pins
    pinMode(this->stepPin, OUTPUT);
    pinMode(this->dirPin, OUTPUT);
    pinMode(this->enablePin, OUTPUT);

    // Enable by default
    this->enable();
}

void StepperController::start()
{
    this->running = true;
}

void StepperController::stop()
{
    this->running = false;
}

void StepperController::spin(int rpm, char direction)
{
    T = setRPM(rpm, resolution);

    if (this->running)
    {
        this->setDirection(direction);

        this->doStep(T);
    }
}

void StepperController::step(int steps, int rpm, int direction)
{
    for (int i = 0; i < steps; ++i)
    {
        this->setDirection(direction);

        this->doStep(T);
    }
}

void StepperController::stepFromAxis(int axisValue, int minRPM, int maxRPM)
{
    // Map analog joystick value to RPM range (minRPM to maxRPM)
    int mappedRPM = map(axisValue, 0, 1023, minRPM, maxRPM);

    // Determine the direction based on joystick value
    int direction = (axisValue > 512) ? 1 : -1; // Assuming joystick center is around 512, adjust if necessary

    // Move the stepper motor
    this->step(1, mappedRPM, direction);
}

void StepperController::enable()
{
    digitalWrite(this->enablePin, HIGH);
}

void StepperController::disable()
{
    digitalWrite(this->enablePin, LOW);
}

void StepperController::setDirection(char direction)
{
    if (direction == 's')
    {
        digitalWrite(this->dirPin, HIGH); // Set direction clockwise (s)
    }
    else if (direction == 'z')
    {
        digitalWrite(this->dirPin, LOW); // Set direction counterclockwise (z)
    }
}

void StepperController::doStep(float T)
{
    digitalWrite(this->stepPin, HIGH);
    delayMicroseconds(T);
    digitalWrite(this->stepPin, LOW);
    delayMicroseconds(T);
}

// This function calculates the period in terms of frequency and returns half of the period vale in microseconds
// from https://github.com/RaulPerezSanchez/Arduino-Stepper-RPM
float StepperController::setRPM(int rpm, float res)
{
    float freq = (float)rpm / ((res / 360) * 60); // calculates the frequency to a related RPM
    float period = 1 / freq;                      // calculates the inverse of the frequency A.K.A the period
    return (period * 0.5) * 1000000;              // returns half of the period in microseconds
}