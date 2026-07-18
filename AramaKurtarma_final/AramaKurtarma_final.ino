/*
 * Arama Kurtarma - Coklu Sensor Sistemi
 * Dosyalar: GPS.ino, Lidar.ino, imu.ino, thermal.ino, webserver.ino
 */

#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>

// ---- Fonksiyon on-bildirimleri (ctags otomatik algilamiyor, elle bildiriyoruz) ----
void handleRoot();
void handleThermalPage();
void handleData_GPS();
void handleData_Lidar();
void handleData_IMU();
void handleData_Thermal();
void handleMarkTarget();
void handleData_Markers();
void handleResetMarkers();
void setupGPS();
void readGPS();
void setupLidar();
void readLidar();
void computeTargetPosition();
void setupIMU();
void readIMU();
void setupThermal();
void readThermal();

// ---- WiFi ----
const char* ssid = "Arslan trnet";
const char* password = "18arslan1834";

// ---- GPS ----
#define GPS_RX_PIN 18
#define GPS_TX_PIN 17
#define GPS_BAUD   9600
HardwareSerial GPSSerial(1);
TinyGPSPlus gps;
double lastLat = 0.0, lastLng = 0.0;
bool gpsHasFix = false;

// ---- LiDAR ----
float lidarDistanceCM = 0.0;
bool lidarOK = false;

// ---- Hedef Hesaplama (LiDAR + GPS + IMU birlesimi) ----
double targetLat = 0.0;
double targetLng = 0.0;
float distanceToTargetM = 0.0;
bool targetValid = false;

// ---- IMU ----
float imuRoll = 0.0, imuPitch = 0.0, imuYaw = 0.0;
bool imuOK = false;

// ---- Manyetometre Kalibrasyon Degerleri ----
int16_t magXmin = 32767, magXmax = -32768;
int16_t magYmin = 32767, magYmax = -32768;

// ---- Isaretlenmis Hedefler ----
#define MAX_MARKERS 20
struct Marker {
  double lat;
  double lng;
  float distance;
  unsigned long timestamp;
};
Marker markers[MAX_MARKERS];
int markerCount = 0;

// ---- Termal ----
float thermalPixels[64];
bool thermalOK = false;

// ---- Web server ----
WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ---- I2C hatti TEK SEFER burada baslatiliyor ----
  // NOT: imu.ino ve thermal.ino icindeki Wire.begin(11,12) satirlarini SIL,
  // cakisma olmasin diye sadece burada bir kez cagriliyor.
  Wire.begin(11, 12);
  Wire.setTimeOut(50);

  setupGPS();       // GPS.ino
  setupLidar();     // Lidar.ino
  setupIMU();       // imu.ino
  setupThermal();   // thermal.ino

  // ---- GECICI I2C TARAYICI ----
  Serial.println("I2C tarama basliyor...");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Cihaz bulundu, adres: 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("HICBIR I2C CIHAZI BULUNAMADI - kablolama/pull-up kontrol et.");
  } else {
    Serial.print("Toplam ");
    Serial.print(found);
    Serial.println(" cihaz bulundu.");
  }
  // ---- TARAYICI SONU ----

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("WiFi'ye baglaniliyor");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Baglandi! IP adresi: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi baglantisi basarisiz!");
  }

  server.on("/", handleRoot);
  server.on("/thermal", handleThermalPage);
  server.on("/data/gps", handleData_GPS);
  server.on("/data/lidar", handleData_Lidar);
  server.on("/data/imu", handleData_IMU);
  server.on("/data/thermal", handleData_Thermal);
  server.on("/mark", handleMarkTarget);
  server.on("/data/markers", handleData_Markers);
  server.on("/reset_markers", handleResetMarkers);

  server.begin();
  Serial.println("Web server baslatildi.");
}

void loop() {
  readGPS();       // GPS.ino
  readLidar();     // Lidar.ino
  readIMU();       // imu.ino
  readThermal();   // thermal.ino

  server.handleClient();
}