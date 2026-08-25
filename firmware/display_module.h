#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

class DisplayModule {
private:
    Adafruit_SSD1306 _display;
    bool _initialized = false;

    void clear() {
        if (!_initialized) return;
        _display.clearDisplay();
        _display.setTextColor(SSD1306_WHITE);
        _display.setTextSize(1);
    }

    void centerText(String text, int y, int size) {
        if (!_initialized) return;
        _display.setTextSize(size);
        int textWidth = text.length() * 6 * size;
        int x = (SCREEN_WIDTH - textWidth) / 2;
        if (x < 0) x = 0;
        _display.setCursor(x, y);
        _display.print(text);
    }

public:
    DisplayModule() : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

    bool begin() {
        _initialized = _display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
        return _initialized;
    }

    void showSplash() {
        if (!_initialized) return;
        clear();
        centerText("Steady", 8, 2);
        centerText("Point", 28, 2);
        centerText("by TEAM ELECTRONAUTS", 52, 1);
        _display.display();
    }

    void showBleSearching() {
        if (!_initialized) return;
        clear();
        centerText("Searching for", 10, 1);
        centerText("Bluetooth", 28, 2);
        centerText("Please wait", 52, 1);
        _display.display();
    }

    void showBleConnected() {
        if (!_initialized) return;
        clear();
        centerText("Bluetooth", 15, 2);
        centerText("Connected", 40, 1);
        _display.display();
    }

    void showBleTimeout() {
        if (!_initialized) return;
        clear();
        centerText("Bluetooth", 15, 2);
        centerText("timeout", 40, 1);
        _display.display();
    }

    void showMenu(int index) {
        if (!_initialized) return;
        clear();
        centerText("== MENU ==", 0, 1);
        _display.setCursor(10, 16);
        _display.print((index == 0) ? "-> " : "   "); _display.print("1. Mouse Mode");
        _display.setCursor(10, 32);
        _display.print((index == 1) ? "-> " : "   "); _display.print("2. Calibrate");
        _display.setCursor(10, 48);
        _display.print((index == 2) ? "-> " : "   "); _display.print("3. Data Record");
        _display.display();
    }

    void showMouseScreen(float gx, float gy, bool scrollMode, int speedLevel, float cutoffHz) {
        if (!_initialized) return;
        clear();
        _display.setCursor(0, 0); _display.println("Mouse Mode");
        _display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
        _display.setCursor(0, 16);
        _display.print("GX: "); _display.println(gx, 1);
        _display.print("GY: "); _display.println(gy, 1);
        _display.setCursor(0, 40);
        _display.print("Mode: "); _display.println(scrollMode ? "SCROLL" : "MOVE");
        _display.print("Speed: "); _display.println(speedLevel);
        
        // --- SPARK Telemetry ---
        _display.setCursor(0, 56);
        if (cutoffHz < 5.0f) {
            _display.print("SPARK: FILTERING");
        } else {
            _display.print("SPARK: TRACKING ");
        }        
        _display.display();
    }

    void showCalibPrompt() {
        if (!_initialized) return;
        clear();
        centerText("Calibration", 10, 2);
        centerText("Press SW to start", 35, 1);
        centerText("Long Press=Back", 50, 1);
        _display.display();
    }

    void showCountdown(int remaining) {
        if (!_initialized) return;
        clear();
        centerText("Calibrating", 5, 1);
        centerText(String(remaining), 18, 3);
        centerText("Hold still...", 50, 1);
        _display.display();
    }

    void showCalibDone() {
        if (!_initialized) return;
        clear();
        centerText("Calibration", 15, 2);
        centerText("Complete!", 40, 1);
        _display.display();
    }
    
    void showSparkProfile(float amp, float freq, float cutoff) {
        if (!_initialized) return;
        clear();
        centerText("SPARK PROFILE", 0, 1);
        _display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
        
        _display.setTextSize(1);
        _display.setCursor(5, 16);
        _display.print("Amplitude: "); _display.println(amp, 2);
        
        _display.setCursor(5, 28);
        _display.print("Frequency: "); _display.print(freq, 1); _display.println(" Hz");
        
        _display.setCursor(5, 40);
        _display.print("AI Cutoff: "); _display.print(cutoff, 1); _display.println(" Hz");
        
        _display.setCursor(20, 54);
        _display.print("[SW] Return Home");
        
        _display.display();
    }

    void showDataScreen(const char* name, unsigned long rem) {
        if (!_initialized) return;
        clear();
        centerText("Data Collection", 0, 1);
        centerText(String(name), 20, 2);
        centerText(String(rem) + "s left", 45, 1);
        centerText("SW/BOOT = Skip", 56, 1);
        _display.display();
    }
};
#endif