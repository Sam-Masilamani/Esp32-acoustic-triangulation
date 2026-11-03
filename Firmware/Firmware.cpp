#include <math.h>

const float ENERGY_MULTIPLIER = 0.008425; 
int THRESHOLD = 500; 

const int S1 = 1, S2 = 2, S3 = 3;
const int S4 = 4, S5 = 5, S6 = 6;
const int S7 = 7, S8 = 8, S9 = 9;

// Physical Coordinates (mm)
const float X_COORD[9] = {20, 60, 140, 20, 100, 180, 60, 140, 180};
const float Y_COORD[9] = {140, 180, 180, 60, 100, 140, 20, 20, 60};

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("System Ready.");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); 
    if (command.startsWith("THRESH:")) {
      THRESHOLD = command.substring(7).toInt();
    }
  }

  // Direct sampling without resting baseline subtraction
  int v1 = analogRead(S1), v2 = analogRead(S2), v3 = analogRead(S3);
  int v4 = analogRead(S4), v5 = analogRead(S5), v6 = analogRead(S6);
  int v7 = analogRead(S7), v8 = analogRead(S8), v9 = analogRead(S9);

  if (v1 > THRESHOLD || v2 > THRESHOLD || v3 > THRESHOLD || 
      v4 > THRESHOLD || v5 > THRESHOLD || v6 > THRESHOLD || 
      v7 > THRESHOLD || v8 > THRESHOLD || v9 > THRESHOLD) {
      
      uint32_t startCycle = ESP.getCycleCount();
      uint32_t t[9] = {0,0,0,0,0,0,0,0,0};
      int p[9] = {v1, v2, v3, v4, v5, v6, v7, v8, v9};
      bool h[9] = {false, false, false, false, false, false, false, false, false};

      if (v1 > THRESHOLD) { t[0] = startCycle; h[0] = true; }
      if (v2 > THRESHOLD) { t[1] = startCycle; h[1] = true; }
      if (v3 > THRESHOLD) { t[2] = startCycle; h[2] = true; }
      if (v4 > THRESHOLD) { t[3] = startCycle; h[3] = true; }
      if (v5 > THRESHOLD) { t[4] = startCycle; h[4] = true; }
      if (v6 > THRESHOLD) { t[5] = startCycle; h[5] = true; }
      if (v7 > THRESHOLD) { t[6] = startCycle; h[6] = true; }
      if (v8 > THRESHOLD) { t[7] = startCycle; h[7] = true; }
      if (v9 > THRESHOLD) { t[8] = startCycle; h[8] = true; }

      uint32_t captureStartTime = micros();
      while(micros() - captureStartTime < 2000) {
        v1 = analogRead(S1); if (!h[0] && v1 > THRESHOLD) { t[0] = ESP.getCycleCount(); h[0] = true; } if (v1 > p[0]) p[0] = v1; 
        v2 = analogRead(S2); if (!h[1] && v2 > THRESHOLD) { t[1] = ESP.getCycleCount(); h[1] = true; } if (v2 > p[1]) p[1] = v2;
        v3 = analogRead(S3); if (!h[2] && v3 > THRESHOLD) { t[2] = ESP.getCycleCount(); h[2] = true; } if (v3 > p[2]) p[2] = v3;
        v4 = analogRead(S4); if (!h[3] && v4 > THRESHOLD) { t[3] = ESP.getCycleCount(); h[3] = true; } if (v4 > p[3]) p[3] = v4;
        v5 = analogRead(S5); if (!h[4] && v5 > THRESHOLD) { t[4] = ESP.getCycleCount(); h[4] = true; } if (v5 > p[4]) p[4] = v5;
        v6 = analogRead(S6); if (!h[5] && v6 > THRESHOLD) { t[5] = ESP.getCycleCount(); h[5] = true; } if (v6 > p[5]) p[5] = v6;
        v7 = analogRead(S7); if (!h[6] && v7 > THRESHOLD) { t[6] = ESP.getCycleCount(); h[6] = true; } if (v7 > p[6]) p[6] = v7;
        v8 = analogRead(S8); if (!h[7] && v8 > THRESHOLD) { t[7] = ESP.getCycleCount(); h[7] = true; } if (v8 > p[7]) p[7] = v8;
        v9 = analogRead(S9); if (!h[8] && v9 > THRESHOLD) { t[8] = ESP.getCycleCount(); h[8] = true; } if (v9 > p[8]) p[8] = v9;
      }

      uint32_t t0 = 4294967295;
      int firstNode = -1;
      for(int i=0; i<9; i++) {
        if (h[i] && t[i] < t0) {
          t0 = t[i];
          firstNode = i;
        }
      }

      int gpios[9] = {S1, S2, S3, S4, S5, S6, S7, S8, S9};
      
      Serial.println("\n[IMPACT DETECTED]");
      Serial.printf("First Triggered Node : Channel %d (GPIO %d)\n", firstNode + 1, gpios[firstNode]);
      Serial.printf("T0 Reference Time    : %lu us\n", micros());
      Serial.println("CH   | GPIO | Peak (mV) | Delta (mV) | TDOA (us)   | Status");
      Serial.println("-----+------+-----------+------------+-------------+---------");
      
      int totalDelta = 0;
      float sumX = 0, sumY = 0;
      int maxTdoa = 0;
      int lastNode = -1;

      for (int i = 0; i < 9; i++) {
        int peak_mV = p[i] * (3300.0 / 4095.0);
        int delta_mV = peak_mV; 
        
        if (h[i]) {
          int tdoa = (t[i] - t0) / 240; 
          if (tdoa > maxTdoa) { maxTdoa = tdoa; lastNode = i; }
          totalDelta += delta_mV;
          sumX += X_COORD[i] * delta_mV;
          sumY += Y_COORD[i] * delta_mV;
          
          String tdoaStr = "+" + String(tdoa);
          Serial.printf("S%d   |  IO%-2d| %7d   |    + %-6d| %11s | HIT\n", i+1, gpios[i], peak_mV, delta_mV, tdoaStr.c_str());
        } else {
          Serial.printf("S%d   |  IO%-2d| %7d   |    + %-6d|         --- | MISS\n", i+1, gpios[i], peak_mV, delta_mV);
        }
      }
      
      float kineticEnergy = totalDelta * ENERGY_MULTIPLIER;
      float cmX = (totalDelta > 0) ? (sumX / totalDelta) : 0;
      float cmY = (totalDelta > 0) ? (sumY / totalDelta) : 0;
      
      float velocity = 0;
      if (lastNode != -1 && maxTdoa > 0) {
        float dx = X_COORD[firstNode] - X_COORD[lastNode];
        float dy = Y_COORD[firstNode] - Y_COORD[lastNode];
        float dist_mm = sqrt((dx*dx) + (dy*dy));
        velocity = (dist_mm / 1000.0) / (maxTdoa / 1000000.0);
      }

      Serial.println("===============================================================");
      Serial.printf("Total Delta Sum      : %d mV\n", totalDelta);
      Serial.printf("Calculated Energy    : %.3f uJ (Micro-Joules)\n", kineticEnergy);
      Serial.printf("Impact Coordinates   : X: %.1f mm, Y: %.1f mm\n", cmX, cmY);
      Serial.printf("Acoustic Velocity    : %.1f m/s\n", velocity);
      Serial.println("===============================================================\n");

      delay(1000); 
  }
}