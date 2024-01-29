#ifndef STEPPERCONTROLLER_H
#define STEPPERCONTROLLER_H

class StepperController
{
private:
    int stepPin;
    int dirPin;
    int enablePin;
    int steps;
    int microsteps;
    int rpm;
    int rpmMicroSteps;
    unsigned int rpm_x;
    bool running;
    float resolution;                           
    float T;                                    // its the inverse of the frequency divided by 2
    sRowAxis rowAxis;
    
    void setDirection(char direction);
    bool doStep(char direction, bool state, bool countTurns = true);

public:
    StepperController(sRowAxis myButton, int stepPin, int dirPin, int enablePin, int microsteps = 32, int steps = 200);

    void start();
    void stop();
    void spin(int rpm, char direction = 's');
    void step(int steps, int rpm, int direction = 's', bool countTurns = true);
    void stepFromAxis(int axisValue, int minRPM, int maxRPM);
    void enable();
    void disable();
};

#endif // STEPPERCONTROLLER_H
