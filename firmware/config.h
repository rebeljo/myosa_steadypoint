#ifndef CONFIG_H
#define CONFIG_H
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
#define I2C_SDA 21
#define I2C_SCL 22
#define BOOT_BUTTON_PIN 0
#define JOY_X_PIN 34
#define JOY_Y_PIN 35
#define JOY_SW_PIN 32
#define SPLASH_TIME_MS        3000
#define BLE_SEARCH_TIMEOUT_MS 15000
#define CAL_COUNTDOWN_SECONDS 10
#define POINTER_HOME_X 500
#define POINTER_HOME_Y 300
#define MOUSE_SENSITIVITY 0.16f
#define GYRO_DEAD_ZONE 4.0f
#endif