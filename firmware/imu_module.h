#ifndef IMU_MODULE_H
#define IMU_MODULE_H
#include <Wire.h>
#include <AccelAndGyro.h>
#include "config.h"
class ImuModule {
private:
  AccelAndGyro _imu; bool _ok=false;
  float readAxis(uint8_t reg){ if(!_ok)return 0; Wire.beginTransmission(MPU6050_ADDRESS_AD0_HIGH); Wire.write(reg);
    if(Wire.endTransmission(true)!=0)return 0; Wire.requestFrom((uint8_t)MPU6050_ADDRESS_AD0_HIGH,(uint8_t)2);
    if (Wire.available() != 2) return 0;
    uint8_t hi = Wire.read(), lo = Wire.read();
    return (float)((int16_t)((hi<<8)|lo))*(250.0f/32768.0f); }
public:
  bool begin(){ Wire.begin(I2C_SDA,I2C_SCL); Wire.setClock(100000);
    for(int i=0;i<3;i++){ if(_imu.begin()||_imu.ping()){_ok=true;return true;} delay(300);} return false; }
  bool isReady(){ return _ok; }

  // *** Y-AXIS INVERSION ***
  // Vertical cursor motion (dy) and vertical scroll (sv) are built from
  // gyro X in your state machine, so the flip lives here in ONE place.
  float getGyroX(){ return -readAxis(MPU6050_GYRO_XOUT_H_REG); }

  float getGyroY(){ return readAxis(MPU6050_GYRO_YOUT_H_REG); }
  float getGyroZ(){ return readAxis(MPU6050_GYRO_ZOUT_H_REG); }
  float applyDeadZone(float v,float dz){ return (v>-dz&&v<dz)?0.0f:v; }
  bool readBlock(int16_t a[3],int16_t g[3]){ if(!_ok)return false;
    Wire.beginTransmission(MPU6050_ADDRESS_AD0_HIGH); Wire.write(MPU6050_ACCEL_XOUT_H_REG);
    if(Wire.endTransmission(false)!=0)return false; Wire.requestFrom((uint8_t)MPU6050_ADDRESS_AD0_HIGH,(uint8_t)14);
    if(Wire.available()<14)return false; uint8_t b[14]; for(int i=0;i<14;i++)b[i]=Wire.read();
    a[0]=(int16_t)((b[0]<<8)|b[1]); a[1]=(int16_t)((b[2]<<8)|b[3]); a[2]=(int16_t)((b[4]<<8)|b[5]);
    g[0]=(int16_t)((b[8]<<8)|b[9]); g[1]=(int16_t)((b[10]<<8)|b[11]); g[2]=(int16_t)((b[12]<<8)|b[13]); return true; }
};
#endif