void setupGPS() {
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS UART baslatildi.");
}

void readGPS() {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }
  if (gps.location.isValid()) {
    lastLat = gps.location.lat();
    lastLng = gps.location.lng();
    gpsHasFix = true;
  }
}

void handleData_GPS() {
  String json = "{";
  json += "\"fix\":" + String(gpsHasFix ? "true" : "false") + ",";
  json += "\"lat\":" + String(lastLat, 6) + ",";
  json += "\"lng\":" + String(lastLng, 6) + ",";
  json += "\"sats\":" + String(gps.satellites.value()) + ",";
  json += "\"alt\":" + String(gps.altitude.isValid() ? gps.altitude.meters() : 0.0, 1) + ",";
  json += "\"speed\":" + String(gps.speed.isValid() ? gps.speed.kmph() : 0.0, 1) + ",";

  String timeStr = "N/A";
  if (gps.time.isValid() && gps.date.isValid()) {
    char buf[30];
    sprintf(buf, "%02d:%02d:%02d - %02d/%02d/%04d",
            gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());
    timeStr = String(buf);
  }
  json += "\"time\":\"" + timeStr + "\"";
  json += "}";
  server.send(200, "application/json", json);
}