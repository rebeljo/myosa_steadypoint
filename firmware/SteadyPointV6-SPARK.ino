#include <Wire.h>
#include <AccelAndGyro.h>
#include <HijelHID_BLEMouse.h>

#include "config.h"
#include "display_module.h"
#include "imu_module.h"
#include "ble_mouse_module.h"
#include "joystick_module.h"
#include "ai_module.h"

// ============================================================
// STATE MACHINE
// ============================================================
class StateMachine {
public:
  enum State { MENU, MOUSE, CALIBRATE, DATA };
  StateMachine(DisplayModule& d,ImuModule& i,BleMouseModule& b,JoystickModule& j,AiModule& a)
    : disp(d),imu(i),ble(b),joy(j),ai(a) {}

  void begin(){
    disp.showSplash();
    unsigned long t0=millis();
    while(millis()-t0<SPLASH_TIME_MS){ if(joy.y()==1||joy.bootDown())break; delay(10); }
    disp.showBleSearching();
    unsigned long t1=millis();
    while(millis()-t1<BLE_SEARCH_TIMEOUT_MS){ if(ble.isConnected())break; if(joy.y()==1||joy.bootDown())break; delay(30); }
    if(ble.isConnected()) disp.showBleConnected(); else disp.showBleTimeout();
    delay(1000);
    state=MENU; disp.showMenu(menuIndex);
  }

  void update(){
    switch(state){
      case MENU: updateMenu(); break;
      case MOUSE: updateMouse(); break;
      case CALIBRATE: updateCalibrate(); break;
      case DATA: updateData(); break;
    }
  }

private:
  DisplayModule& disp; ImuModule& imu; BleMouseModule& ble; JoystickModule& joy; AiModule& ai;
  State state=MENU;
  int menuIndex=0, speedIndex=1; bool scrollMode=false;
  const float speeds[3]={0.08f,0.16f,0.28f};
  int lastX=0,lastY=0; bool lastSw=false; unsigned long swT=0; bool swLongFired=false;

  int swEvent(){
    bool s=joy.swDown(); int ev=0;
    if(s&&!lastSw){ swT=millis(); swLongFired=false; }
    if(s&&!swLongFired&&(millis()-swT)>=2000){ swLongFired=true; ev=2; }
    if(!s&&lastSw){ if(!swLongFired&&(millis()-swT)<800) ev=1; swLongFired=false; }
    lastSw=s; return ev;
  }

  void updateMenu(){
    int y=joy.y();
    if(y!=lastY){
      if(y==1)  menuIndex=(menuIndex+1)%3;   
      if(y==-1) menuIndex=(menuIndex+2)%3;   
      lastY=y; disp.showMenu(menuIndex);
    }
    if(swEvent()==1){
      if(menuIndex==0)state=MOUSE; else if(menuIndex==1)state=CALIBRATE; else state=DATA;
    }
  }

  void updateMouse(){
    int ev=swEvent();
    if(ev==1){ scrollMode=!scrollMode; }
    if(ev==2){ state=MENU; disp.showMenu(menuIndex); return; }
    
    // CRITICAL FIX: Read raw data and feed the AI window so the filter works
    float raw_gx = imu.getGyroX();
    float raw_gy = imu.getGyroY();
    float raw_gz = imu.getGyroZ();
    ai.update(raw_gx, raw_gy, raw_gz);

    int y=joy.y();
    if(y!=lastY){
      if(y==-1) ble.resetPointer();            
      if(y==1)  speedIndex=(speedIndex+1)%3;   
      lastY=y;
    }
    int x=joy.x();
    if(x!=lastX){ if(x==-1)ble.clickLeft(); if(x==1)ble.clickRight(); lastX=x; }
    
    float gx=imu.applyDeadZone(ai.filterX(raw_gx),GYRO_DEAD_ZONE);
    float gy=imu.applyDeadZone(ai.filterY(raw_gy),GYRO_DEAD_ZONE);
    float s=speeds[speedIndex];
    
    if(scrollMode){ 
        int8_t sv=(int8_t)constrain((int)(gx*s*2),-127,127); 
        if(sv)ble.addScroll(sv); 
    } else { 
        int dx=(int)(gy*s), dy=(int)(gx*s); 
        if(dx||dy)ble.move(dx,dy); 
    }
    disp.showMouseScreen(gx, gy, scrollMode, speedIndex + 1,ai.getCurrentCutoff());
  }

  void updateCalibrate(){
    disp.showCalibPrompt();
    while(true){ int ev=swEvent(); if(ev==1)break; if(ev==2||joy.bootDown()){state=MENU;disp.showMenu(menuIndex);return;} delay(10); }
    
    ai.beginCalibration();
    unsigned long t0=millis(); bool skipped=false;
    while(millis()-t0<(unsigned long)CAL_COUNTDOWN_SECONDS*1000UL)
    {
      int rem=CAL_COUNTDOWN_SECONDS-(millis()-t0)/1000;
      disp.showCountdown(rem);
      ai.addSample(imu.getGyroX(),imu.getGyroY(),imu.getGyroZ());
      if(swEvent()||joy.bootDown()){ skipped=true; break; }
      delay(20);
    }
    ai.endCalibration();
    
    // --- NEW: Show SPARK Profile and wait for SW press ---
    while(true) 
    { 
        disp.showSparkProfile( ai.getPersonalizedAmplitude(), ai.getPersonalizedFrequency(), ai.getCurrentCutoff());
        // Exit loop when user short-presses SW or Boot
        if (swEvent() == 1 || joy.bootDown()) break; 
        delay(50);
    }
    
    state=MENU; 
    disp.showMenu(menuIndex);
  }   

  void updateData(){
    Serial.println("timestamp_ms,label,ax,ay,az,gx,gy,gz,gx_dps,gy_dps,gz_dps");
    const int NP=11;
    int labels[NP]={99,1,99,2,99,3,99,4,99,5,99};
    const char* names[NP]={"GET READY","STILL TABLE","GET READY","STILL HAND","GET READY","SLOW","GET READY","FAST","GET READY","VIBRATION","DONE"};
    unsigned long dur[NP]={3000,10000,3000,10000,3000,10000,3000,10000,3000,10000,2000};
    int ph=0; unsigned long phT=millis(), lastS=0;
    const float GS=250.0f/32768.0f;
    while(ph<NP){
      if(swEvent()||joy.bootDown()){ ph++; phT=millis(); if(ph>=NP)break; }
      if(millis()-phT>=dur[ph]){ ph++; phT=millis(); if(ph>=NP)break; }
      if(millis()-lastS>=10){
        int16_t a[3],g[3];
        if(imu.readBlock(a,g))
          Serial.printf("%lu,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f\n",(unsigned long)millis(),labels[ph],a[0],a[1],a[2],g[0],g[1],g[2],g[0]*GS,g[1]*GS,g[2]*GS);
        lastS=millis();
      }
      unsigned long el=millis()-phT; unsigned long rem=(dur[ph]>el)?(dur[ph]-el):0;
      disp.showDataScreen(names[ph],rem/1000);
      delay(1);
    }
    state=MENU; disp.showMenu(menuIndex);
  }
};

// ============================================================
// INSTANCES & MAIN LOOP
// ============================================================
DisplayModule  disp;
ImuModule      imu;
BleMouseModule ble;
JoystickModule joy;
AiModule       ai;
StateMachine   sm(disp, imu, ble, joy, ai);

void setup() {
  Serial.begin(115200);
  disp.begin();
  imu.begin();
  joy.begin();
  ble.begin();
  sm.begin();      // splash -> BLE search -> menu
}

void loop() {
  sm.update();     // state machine handles everything
}