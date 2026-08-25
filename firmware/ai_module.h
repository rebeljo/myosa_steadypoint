#ifndef AI_MODULE_H
#define AI_MODULE_H
#include <Arduino.h>
#include <math.h>

#define SPARK_PI 3.14159265358979f

// ==========================================================
// SPARK: SteadyPoint Adaptive Response Kernel
// Personalized AI engine: learns YOUR tremor, then adapts live
// ==========================================================
class AiModule {
private:
  // --- Personalized user profile (learned in calibration) ---
  float userBias[3] = {0,0,0};
  float userTremorAmplitude = 2.0f;
  float userTremorFrequency = 6.0f;

  // --- Adaptive biquad low-pass (Direct Form I) ---
  float x1[2]={0,0}, x2[2]={0,0}, y1[2]={0,0}, y2[2]={0,0};
  float b0=1,b1=0,b2=0,a1=0,a2=0;
  float currentCutoff = 2.0f;
  const float MIN_CUTOFF = 1.5f;
  const float MAX_CUTOFF = 12.0f;
  const float SAMPLE_RATE = 100.0f;

  // --- Live window for intent-vs-tremor detection ---
  static const int W = 20;
  float wX[W], wY[W]; int wIdx=0;

  // --- Calibration buffers ---
  static const int CAL_SAMPLES = 500;
  float calX[CAL_SAMPLES], calY[CAL_SAMPLES], calZ[CAL_SAMPLES];
  int calIdx = 0; bool calibrating = false;

  void computeCoefficients(float fc) {
    float omega = 2.0f * SPARK_PI * fc / SAMPLE_RATE;
    float sn = sinf(omega), cs = cosf(omega);
    float alpha = sn / 1.4142f;
    float a0 = 1.0f + alpha;
    b0 = ((1.0f - cs) / 2.0f) / a0;
    b1 = (1.0f - cs) / a0;
    b2 = b0;
    a1 = (-2.0f * cs) / a0;
    a2 = (1.0f - alpha) / a0;
  }

  float applyBiquad(float in, int axis) {
    float out = b0*in + b1*x1[axis] + b2*x2[axis] - a1*y1[axis] - a2*y2[axis];
    x2[axis]=x1[axis]; x1[axis]=in;
    y2[axis]=y1[axis]; y1[axis]=out;
    return out;
  }

    float estimateFrequency(float* buf, int len) {
    if (len < 20) return 6.0f;

    // mean
    float mean = 0;
    for (int i = 0; i < len; i++) mean += buf[i];
    mean /= len;

    // hysteresis band = 25% of peak swing (ignores noise wiggles)
    float maxDev = 0;
    for (int i = 0; i < len; i++) {
      float d = fabsf(buf[i] - mean);
      if (d > maxDev) maxDev = d;
    }
    float hyst = maxDev * 0.25f;
    if (hyst < 0.6f) hyst = 0.6f;   // noise floor deadband

    // count only full up<->down swings that cross the band
    int flips = 0;
    int state = 0;
    for (int i = 1; i < len - 1; i++) {
      float v = (buf[i-1] + buf[i] + buf[i+1]) / 3.0f;  // 3-sample smoothing
      float d = v - mean;
      if (d >  hyst) { if (state == -1) flips++; state =  1; }
      if (d < -hyst) { if (state ==  1) flips++; state = -1; }
    }
    // 2 flips = 1 full cycle
    return (flips / 2.0f) / (len / SAMPLE_RATE);
  }

public:
  void begin() { computeCoefficients(MIN_CUTOFF); }

  // ---------- SPARK Phase 1: Personalized calibration ----------
  void beginCalibration() { calIdx=0; calibrating=true; }

  void addSample(float gx, float gy, float gz) {
    if (!calibrating) return;
    if (calIdx < CAL_SAMPLES) { calX[calIdx]=gx; calY[calIdx]=gy; calZ[calIdx]=gz; calIdx++; }
  }

  void endCalibration() {
    calibrating = false;
    if (calIdx < 100) {
      userTremorAmplitude = 2.0f; userTremorFrequency = 6.0f;
    } else {
      float sx=0,sy=0,sz=0;
      for(int i=0;i<calIdx;i++){ sx+=calX[i]; sy+=calY[i]; sz+=calZ[i]; }
      userBias[0]=sx/calIdx; userBias[1]=sy/calIdx; userBias[2]=sz/calIdx;

      float var=0;
      for(int i=0;i<calIdx;i++){
        float dx=calX[i]-userBias[0], dy=calY[i]-userBias[1], dz=calZ[i]-userBias[2];
        var += dx*dx+dy*dy+dz*dz;
      }
      userTremorAmplitude = sqrtf(var/calIdx);
      if (userTremorAmplitude < 1.0f) userTremorAmplitude = 1.0f;

      float fx=estimateFrequency(calX,calIdx), fy=estimateFrequency(calY,calIdx);
      userTremorFrequency = (fx + fy) / 2.0f;
      if (userTremorFrequency < 2.0f)  userTremorFrequency = 2.0f;
      if (userTremorFrequency > 12.0f) userTremorFrequency = 12.0f;
    }
    currentCutoff = userTremorFrequency * 0.4f;
    if (currentCutoff < MIN_CUTOFF) currentCutoff = MIN_CUTOFF;
    if (currentCutoff > 8.0f) currentCutoff = 8.0f;
    computeCoefficients(currentCutoff);

  }

  // ---------- SPARK Phase 2: Adaptive response engine ----------
  void update(float gx, float gy, float gz) {
    float cx = gx - userBias[0];
    float cy = gy - userBias[1];
    wX[wIdx]=cx; wY[wIdx]=cy; wIdx=(wIdx+1)%W;

    // mean (sustained intent) vs std (jitter/tremor)
    float mx=0,my=0;
    for(int i=0;i<W;i++){ mx+=wX[i]; my+=wY[i]; }
    mx/=W; my/=W;
    float sustained = sqrtf(mx*mx+my*my);
    float var=0;
    for(int i=0;i<W;i++){ float dx=wX[i]-mx, dy=wY[i]-my; var+=dx*dx+dy*dy; }
    float jitter = sqrtf(var/W);

    // Intentional (sustained >> jitter) opens the filter; tremor keeps it closed
    float drive = sustained / (jitter + userTremorAmplitude);
    float target = MIN_CUTOFF + (MAX_CUTOFF-MIN_CUTOFF) * (drive>1.0f?1.0f:drive);
    currentCutoff = 0.9f*currentCutoff + 0.1f*target;
    computeCoefficients(currentCutoff);
  }

  float filterX(float v){ return applyBiquad(v - userBias[0], 0); }
  float filterY(float v){ return applyBiquad(v - userBias[1], 1); }
  float filterZ(float v){ return v - userBias[2]; }

  // Telemetry for OLED / Serial
  float getPersonalizedAmplitude(){ return userTremorAmplitude; }
  float getPersonalizedFrequency(){ return userTremorFrequency; }
  float getCurrentCutoff(){ return currentCutoff; }
};
#endif // AI_MODULE_H