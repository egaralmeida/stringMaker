#include <Arduino.h>
#include "config.h"
#include "stepperController.h"

StepperController::StepperController(sRowAxis rowAxis, int stepPin, int dirPin, int enablePin, int microsteps, int steps)
{
    this->stepPin = stepPin;
    this->dirPin = dirPin;
    this->enablePin = enablePin;
    this->microsteps = microsteps; // 32 default
    this->steps = steps;           // 200 default
    this->running = false;
    this->rpm = 0;
    this->rpmMicroSteps = 0;
    this->rpm_x = 0;
    this->rowAxis = rowAxis;

    this->resolution = (float)360 / (float)(this->steps * this->microsteps); // resolution is constant after this

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
    if (this->running)
    {
        this->setDirection(direction);
        this->rpmMicroSteps = rpm * rpmMicroSteps;
        this->rpm_x = 300000 / rpmMicroSteps;
        while(this->doStep(direction, micros() % this->rpm_x < 100, true)) {
        
        }
    }
}

void StepperController::step(int steps, int rpm, int direction, bool countTurns = true)
{
    for (int i = 0; i < steps; ++i)
    {
        this->setDirection(direction);
        this->rpmMicroSteps = rpm * this->microsteps;
        this->rpm_x = 300000 / rpmMicroSteps;
        
        while (!this->doStep(direction, micros() % this->rpm_x < 100, countTurns)) {};
    }
}

void StepperController::stepFromAxis(int axisValue, int minRPM, int maxRPM)
{
    // Map analog joystick value to RPM range (minRPM to maxRPM)
    int mappedRPM = map(axisValue, 0, 1023, minRPM, maxRPM);

    // Determine the direction based on joystick value
    int direction = (axisValue > 512) ? 1 : -1; // TODO adjust if necessary, the joystick might be misaligned

    // Move the stepper motor
    this->step(1, mappedRPM, direction, false);
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

bool StepperController::doStep(char direction, bool state, bool countTurns = true)
{
    if (state)
    {
        digitalWrite(this->stepPin, HIGH);
        Serial.print(1);
        return false;
    }
    else
    {
        Serial.print(0);
        digitalWrite(this->stepPin, LOW);

        // Update rotations for row axis
        if (countTurns)
        {
            if (direction == 's')
            {
                rowAxis.turnsS++;
            }
            else if (direction == 'z')
            {
                rowAxis.turnsZ++;
            }
        }

        return true;
    }
}
