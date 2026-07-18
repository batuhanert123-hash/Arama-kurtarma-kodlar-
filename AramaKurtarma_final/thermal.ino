#include <Adafruit_AMG88xx.h>

Adafruit_AMG88xx amg;

void setupThermal() {
  // NOT: Wire.begin(11,12) ANA dosyada TEK SEFER cagriliyor, burada tekrar cagirma.
  // AMG8833'un I2C adresi taramada 0x69 olarak bulunmustu (ADDR pini HIGH).
  if (amg.begin(0x69)) {
    thermalOK = true;
    Serial.println("AMG8833 (termal kamera) baslatildi.");
  } else {
    thermalOK = false;
    Serial.println("AMG8833 baslatma HATASI - adres/kablo kontrol et.");
  }
}

void readThermal() {
  if (!thermalOK) return;

  amg.readPixels(thermalPixels); // 64 elemanli float dizisine 8x8 sicaklik verisini yazar (Celsius)
}

void handleData_Thermal() {
  String json = "{";
  json += "\"ok\":" + String(thermalOK ? "true" : "false") + ",";
  json += "\"pixels\":[";
  for (int i = 0; i < 64; i++) {
    json += String(thermalPixels[i], 1);
    if (i < 63) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}