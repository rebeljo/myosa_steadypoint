#ifndef BLE_MOUSE_MODULE_H
#define BLE_MOUSE_MODULE_H
#include <HijelHID_BLEMouse.h>
#include "config.h"

class BleMouseModule {
private:
    HijelBLEMouse _mouse;
public:
    BleMouseModule() : _mouse("SteadyPoint", "Hijel") {}
    
    void begin() { _mouse.begin(); }
    bool isConnected() { return _mouse.isConnected(); }
    
    void move(int deltaX, int deltaY) {
        if (isConnected()) _mouse.move(deltaX, deltaY);
    }
    
    void addScroll(int8_t scroll) {
        if (isConnected()) _mouse.addScroll(scroll);
    }
    
    void clickLeft() {
        if (isConnected()) _mouse.click(MouseButton::Left);
    }
    
    void clickRight() {
        if (isConnected()) _mouse.click(MouseButton::Right);
    }
    
    void resetPointer() {
        if (!isConnected()) return;
        for (int i = 0; i < 25; i++) {
            _mouse.move(-1000, -1000);
            delay(3);
        }
        _mouse.move(POINTER_HOME_X / 10, POINTER_HOME_Y / 10);
    }
};
#endif