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
    this->prevRpm = 0;
    this->prevDirection = 'x';
    this->rowAxis = rowAxis;
    this->currentSteps = 0;

    // AccelStepper stepper(AccelStepper::DRIVER, this->stepPin, this->dirPin);

    // Initialize pins
    pinMode(this->stepPin, OUTPUT);
    pinMode(this->dirPin, OUTPUT);
    pinMode(this->enablePin, OUTPUT);

    // Enable by default
    // this->enable();
}

void StepperController::start()
{
    if (!this->running)
    {
        this->running = true;
        this->enable();
    }
}

void StepperController::stop()
{
    if (this->running)
    {
        this->running = false;
        this->disable();
    }
}

void StepperController::spin(int rpm, char direction, bool countTurns)
{
    // Serial.println("spin");
    if (this->running)
    {
        // Serial.println("spin running");
        this->setDir(direction);

        if (rpm != prevRpm)
        {
            // Serial.println("spin set rpm");
            setRPM(rpm);
            prevRpm = rpm;
        }

        if (stepper.runSpeed())
        {
            // Serial.println("Runspeed");
            if (countTurns)
            {
                if (direction == 's')
                {
                    // Serial.print("spin runspeed direction = ");
                    // Serial.println(direction);

                    currentSteps--;
                    if (currentSteps < -steps)
                    {
                        Serial.println("revolution");
                        rowAxis.turnsS++;
                        currentSteps = 0;
                    }
                }
                else if (direction == 'z')
                {
                    // Serial.print("spin runspeed direction = ");
                    Serial.println(direction);

                    currentSteps++;
                    if (currentSteps > steps)
                    {
                        Serial.println("revolution");
                        rowAxis.turnsZ++;
                        currentSteps = 0;
                    }
                }
            }
        }
    }
    else
    {
        // stepper.setSpeed(0);
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
    digitalWrite(this->enablePin, LOW);
}

void StepperController::disable()
{
    // digitalWrite(this->enablePin, HIGH);
}

void StepperController::setDir(char direction)
{

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
