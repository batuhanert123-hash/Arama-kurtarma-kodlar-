/*
 * TF-Luna LiDAR + Hedef Isaretleme
 * UART2: GPIO20 TX, GPIO19 RX
 */

#define LIDAR_RX_PIN 19
#define LIDAR_TX_PIN 20
#define LIDAR_BAUD   115200

HardwareSerial LidarSerial(2);

void setupLidar() {
  LidarSerial.begin(LIDAR_BAUD, SERIAL_8N1, LIDAR_RX_PIN, LIDAR_TX_PIN);
  Serial.println("LiDAR UART baslatildi.");
}

void readLidar() {
  while (LidarSerial.available() >= 9) {
    if (LidarSerial.read() == 0x59) {
      if (LidarSerial.peek() == 0x59) {
        LidarSerial.read();

        uint8_t frame[7];
        LidarSerial.readBytes(frame, 7);

        uint16_t distCM = frame[0] | (frame[1] << 8);
        uint16_t strength = frame[2] | (frame[3] << 8);

        uint8_t checksum = 0x59 + 0x59;
        for (int i = 0; i < 6; i++) {
          checksum += frame[i];
        }
        uint8_t receivedChecksum = frame[6];

        if (checksum == receivedChecksum && strength > 100) {
          lidarDistanceCM = distCM;
          lidarOK = true;
        } else {
          lidarOK = false;
        }
      }
    }
  }

  computeTargetPosition();
}

void computeTargetPosition() {
  if (!gpsHasFix || !lidarOK || !imuOK) {
    targetValid = false;
    return;
  }

  const double R = 6371000.0;
  double distM = lidarDistanceCM / 100.0;

  if (distM < 0.1 || distM > 800.0) {
    targetValid = false;
    return;
  }

  double lat1 = lastLat * PI / 180.0;
  double lon1 = lastLng * PI / 180.0;
  double bearing = imuYaw * PI / 180.0;

  double angularDist = distM / R;

  double lat2 = asin(sin(lat1) * cos(angularDist) +
                      cos(lat1) * sin(angularDist) * cos(bearing));

  double lon2 = lon1 + atan2(sin(bearing) * sin(angularDist) * cos(lat1),
                              cos(angularDist) - sin(lat1) * sin(lat2));

  targetLat = lat2 * 180.0 / PI;
  targetLng = lon2 * 180.0 / PI;
  distanceToTargetM = distM;
  targetValid = true;
}

void handleData_Lidar() {
  String json = "{";
  json += "\"ok\":" + String(lidarOK ? "true" : "false") + ",";
  json += "\"distance\":" + String(lidarDistanceCM, 1) + ",";
  json += "\"targetValid\":" + String(targetValid ? "true" : "false") + ",";
  json += "\"targetLat\":" + String(targetLat, 6) + ",";
  json += "\"targetLng\":" + String(targetLng, 6) + ",";
  json += "\"distanceToTarget\":" + String(distanceToTargetM, 1);
  json += "}";
  server.send(200, "application/json", json);
}

void handleMarkTarget() {
  if (targetValid && markerCount < MAX_MARKERS) {
    markers[markerCount].lat = targetLat;
    markers[markerCount].lng = targetLng;
    markers[markerCount].distance = distanceToTargetM;
    markers[markerCount].timestamp = millis();
    markerCount++;

    server.send(200, "application/json", "{\"success\":true,\"count\":" + String(markerCount) + "}");
  } else if (markerCount >= MAX_MARKERS) {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"Maksimum isaret sayisina ulasildi\"}");
  } else {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"Hedef henuz gecerli degil\"}");
  }
}

void handleData_Markers() {
  String json = "{\"markers\":[";
  for (int i = 0; i < markerCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"lat\":" + String(markers[i].lat, 6) + ",";
    json += "\"lng\":" + String(markers[i].lng, 6) + ",";
    json += "\"distance\":" + String(markers[i].distance, 1) + ",";
    json += "\"id\":" + String(i + 1);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleResetMarkers() {
  markerCount = 0;
  server.send(200, "application/json", "{\"success\":true}");
}