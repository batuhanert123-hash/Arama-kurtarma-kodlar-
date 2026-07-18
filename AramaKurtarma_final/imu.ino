#include <Wire.h>
#include <math.h>

#define ADXL345_ADDR   0x53
#define QMC5883L_ADDR  0x0D

bool adxlFound = false;
bool qmcFound = false;

void setupIMU() {
  // NOT: Wire.begin(11,12) ANA dosyada TEK SEFER cagriliyor, burada TEKRAR CAGIRMA.

  // ---- ADXL345 dene ----
  Wire.beginTransmission(ADXL345_ADDR);
  byte errAdxl = Wire.endTransmission();
  if (errAdxl == 0) {
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x2D);  // POWER_CTL
    Wire.write(0x08);  // Measure mode
    Wire.endTransmission();
    adxlFound = true;
    Serial.println("ADXL345 (ivmeolcer) bulundu ve baslatildi.");
  } else {
    Serial.println("ADXL345 (ivmeolcer) BULUNAMADI - Roll/Pitch devre disi.");
  }

  // ---- QMC5883L dene ----
  Wire.beginTransmission(QMC5883L_ADDR);
  byte errQmc = Wire.endTransmission();
  if (errQmc == 0) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(0x0B);  // Set/Reset period register
    Wire.write(0x01);
    Wire.endTransmission();

    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(0x09);  // Control register 1
    Wire.write(0x1D);  // OSR=512, RNG=8G, ODR=200Hz, Mode=Continuous
    Wire.endTransmission();

    qmcFound = true;
    Serial.println("QMC5883L (manyetometre) bulundu ve baslatildi.");
  } else {
    Serial.println("QMC5883L (manyetometre) BULUNAMADI.");
  }

  if (!adxlFound && !qmcFound) {
    Serial.println("IMU baslatma HATASI - hicbir sensor bulunamadi.");
  }
}

void readIMU() {
  bool ok = false;

  // ---- ADXL345 (ivmeolcer) DEVRE DISI - sadece Yaw kullaniliyor ----
  /*
  if (adxlFound) {
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(0x32);
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom(ADXL345_ADDR, (uint8_t)6);
      if (Wire.available() >= 6) {
        int16_t ax = Wire.read() | (Wire.read() << 8);
        int16_t ay = Wire.read() | (Wire.read() << 8);
        int16_t az = Wire.read() | (Wire.read() << 8);

        float axf = ax / 256.0;
        float ayf = ay / 256.0;
        float azf = az / 256.0;

        imuRoll  = atan2(ayf, azf) * 180.0 / PI;
        imuPitch = atan2(-axf, sqrt(ayf * ayf + azf * azf)) * 180.0 / PI;
        ok = true;
      }
    }
  }
  */

  // ---- QMC5883L (manyetometre) - TEK KULLANILAN SENSOR ----
  if (qmcFound) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom(QMC5883L_ADDR, (uint8_t)6);
      if (Wire.available() >= 6) {
        int16_t mx = Wire.read() | (Wire.read() << 8);
        int16_t my = Wire.read() | (Wire.read() << 8);
        int16_t mz = Wire.read() | (Wire.read() << 8);

        if (mx < magXmin) magXmin = mx;
        if (mx > magXmax) magXmax = mx;
        if (my < magYmin) magYmin = my;
        if (my > magYmax) magYmax = my;

        float offsetX = (magXmax + magXmin) / 2.0;
        float offsetY = (magYmax + magYmin) / 2.0;
        float correctedX = mx - offsetX;
        float correctedY = my - offsetY;

        float heading = atan2(correctedY, correctedX) * 180.0 / PI;
        if (heading < 0) heading += 360.0;
        imuYaw = heading;
        ok = true;

        // ---- DEBUG (gerekirse yorum satirlarini kaldirip tekrar ac) ----
        // Serial.print("[IMU] mx:"); Serial.print(mx);
        // Serial.print(" my:"); Serial.print(my);
        // Serial.print(" | minX:"); Serial.print(magXmin);
        // Serial.print(" maxX:"); Serial.print(magXmax);
        // Serial.print(" minY:"); Serial.print(magYmin);
        // Serial.print(" maxY:"); Serial.print(magYmax);
        // Serial.print(" | Yaw:"); Serial.println(imuYaw, 1);
      }
    }
  }

  imuOK = ok;
}

void handleData_IMU() {
  String json = "{";
  json += "\"ok\":" + String(imuOK ? "true" : "false") + ",";
  json += "\"roll\":" + String(imuRoll, 1) + ",";
  json += "\"pitch\":" + String(imuPitch, 1) + ",";
  json += "\"yaw\":" + String(imuYaw, 1) + ",";
  json += "\"accelOK\":" + String(adxlFound ? "true" : "false") + ",";
  json += "\"magOK\":" + String(qmcFound ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}