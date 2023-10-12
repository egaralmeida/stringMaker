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
    bool running;
    float resolution;                           
    float T;                                    // its the inverse of the frequency divided by 2

    void setDirection(char direction);
    void doStep(float T);
    float setRPM(int rpm, float res);

public:
    StepperController(int stepPin, int dirPin, int enablePin, int microsteps = 16, int steps = 200);

    void start();
    void stop();
    void spin(int rpm, char direction = 's');
    void step(int steps, int rpm, int direction = 's');
    void stepFromAxis(int axisValue, int minRPM, int maxRPM);
    void enable();
    void disable();
};

#endif // STEPPERCONTROLLER_H
