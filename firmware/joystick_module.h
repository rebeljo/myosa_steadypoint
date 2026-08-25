#ifndef JOYSTICK_MODULE_H
#define JOYSTICK_MODULE_H
#include <Arduino.h>
#include "config.h"

class JoystickModule {
public:
    void begin() {
        pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
        pinMode(JOY_SW_PIN, INPUT_PULLUP);
    }

    // Returns -1 (UP/LEFT), 0 (CENTER), 1 (DOWN/RIGHT)
    int y() {
        int val = analogRead(JOY_Y_PIN);
        if (val < 1000) return -1; // UP
        if (val > 3000) return 1;  // DOWN
        return 0;
    }

    int x() {
        int val = analogRead(JOY_X_PIN);
        if (val < 1000) return -1; // LEFT
        if (val > 3000) return 1;  // RIGHT
        return 0;
    }

    bool swDown() {
        return digitalRead(JOY_SW_PIN) == LOW;
    }

    bool bootDown() {
        return digitalRead(BOOT_BUTTON_PIN) == LOW;
    }
};
#endif