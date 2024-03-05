#ifndef STEPPERCONTROLLER_H
#define STEPPERCONTROLLER_H
#include <AccelStepper.h>
#include <MultiStepper.h>

class StepperController
{
private:
    int stepPin;
    int dirPin;
    int enablePin;
    int steps;
    int microsteps;
    int rpm;
    int prevRpm;
    char prevDirection;
    bool running;
    float resolution;
    int currentSteps;         
    sRowAxis rowAxis;
    AccelStepper stepper;
    
    void setDir(char direction);
    void setRPM(int rpm);
    bool doStep(char direction, bool state, bool countTurns = true);

public:
    StepperController(sRowAxis myButton, int stepPin, int dirPin, int enablePin, int microsteps = 32, int steps = 200);

    void start();
    void stop();
    void spin(int rpm, char direction = 's', bool countTurns = true);
    void step(int steps, int rpm, int direction = 's', bool countTurns = false);
    void stepFromAxis(int axisValue, int minRPM, int maxRPM);
    void enable();
    void disable();
};

#endif // STEPPERCONTROLLER_H
