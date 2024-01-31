#include <Arduino.h>
#include "config.h"
#include "stepperController.h"

StepperController::StepperController(sRowAxis rowAxis, int stepPin, int dirPin, int enablePin, int microsteps, int steps)
    : stepper(AccelStepper::DRIVER, stepPin, dirPin) 
{
    this->stepPin = stepPin;
    this->dirPin = dirPin;
    this->enablePin = enablePin;
    this->microsteps = microsteps;
    this->steps = steps * microsteps;
    this->running = false;
    this->rpm = 0;
    this->rowAxis = rowAxis;
    this->currentSteps = 0;

    //AccelStepper stepper(AccelStepper::DRIVER, this->stepPin, this->dirPin);

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

void StepperController::spin(int rpm, char direction, bool countTurns)
{

    static int prevRpm = 0;

    if (this->running)
    {
        this->setDirection(direction);
        if (rpm != prevRpm)
        {
            setRPM(rpm);
            prevRpm = rpm;
        }

        if (stepper.runSpeed())
        {
            if (countTurns)
            {
                if (direction == 's')
                {
                    currentSteps--;
                    if (currentSteps < -steps)
                    {
                        rowAxis.turnsS++;
                        currentSteps = 0;
                    }
                }
                else if (direction == 'z')
                {
                    currentSteps++;
                    if (currentSteps > steps)
                    {
                        rowAxis.turnsZ++;
                        currentSteps = 0;
                    }
                }
            }
        }
    }
}

void StepperController::step(int steps, int rpm, int direction, bool countTurns)
{
}

void StepperController::stepFromAxis(int axisValue, int minRPM, int maxRPM)
{
    // Map analog joystick value to RPM range (minRPM to maxRPM)
    int mappedRPM = map(axisValue, 0, 1023, minRPM, maxRPM);

    // Determine the direction based on joystick value
    int direction = (axisValue > 512) ? 1 : -1; // TODO adjust if necessary, the joystick might be misaligned

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
    static char prevDirection = direction;

    if (direction != prevDirection) // Don't change direction if its the same.
    {
        if (direction == 's')
        {
            digitalWrite(this->dirPin, HIGH); // Set direction clockwise (s)
            
        }
        else if (direction == 'z')
        {
            digitalWrite(this->dirPin, LOW); // Set direction counterclockwise (z)
        }

        prevDirection = direction;
    }
}

void StepperController::setRPM(int rpm)
{
    // Convert RPM to steps per second
    float stepsPerSecond = (rpm * this->steps) / 60;
    this->stepper.setSpeed(stepsPerSecond);
}
/*
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
}*/
