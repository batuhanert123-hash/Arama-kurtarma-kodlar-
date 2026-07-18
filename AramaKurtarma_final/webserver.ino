void handleRoot() {
  String html = R"WEBGUI2726(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Arama Kurtarma - Sensor Paneli</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<style>
  * { box-sizing: border-box; }
  body { font-family: Arial, sans-serif; background:#1a1a1a; color:#eee; margin:0; padding:15px; }
  h1 { color:#4CAF50; font-size:18px; margin:0 0 12px 0; }
  a.thermal-link {
    display:inline-block; background:#e67e22; color:#111; text-decoration:none;
    padding:8px 14px; border-radius:8px; font-weight:bold; font-size:13px; margin-bottom:15px;
  }
  .layout { display:flex; gap:15px; align-items:flex-start; }
  #map { flex:1; width:100%; height:70vh; border-radius:10px; }
  .sidebar { width:220px; flex-shrink:0; }
  .card { background:#2a2a2a; border-radius:8px; padding:10px 12px; margin-bottom:10px; }
  .label { color:#999; font-size:11px; }
  .value { font-size:16px; font-weight:bold; color:#4CAF50; word-break:break-all; }
  .status-ok { color:#4CAF50; }
  .status-bad { color:#e74c3c; }
  .bottom-row { display:flex; gap:15px; margin-top:15px; flex-wrap:wrap; }
  .bottom-card { flex:1; min-width:220px; background:#2a2a2a; border-radius:10px; padding:15px; }
  .bottom-card h2 { font-size:14px; color:#4CAF50; margin:0 0 10px 0; }
  .mini-value { font-size:22px; font-weight:bold; color:#eee; }
  button { font-family: Arial, sans-serif; }
  @media (max-width: 700px) {
    .layout { flex-direction: column; }
    .sidebar { width:100%; display:flex; flex-wrap:wrap; gap:10px; }
    .sidebar .card { flex:1; min-width:140px; }
    #map { height:50vh; }
  }
</style>
</head>
<body>
  <h1>Arama Kurtarma - Sensor Paneli</h1>
  <a class="thermal-link" href="/thermal">Termal Kamera Sayfasini Ac</a>

  <div class="layout">
    <div id="map"></div>
    <div class="sidebar">
      <div class="card"><div class="label">Fix Durumu</div><div class="value" id="fixStatus">-</div></div>
      <div class="card"><div class="label">Enlem / Boylam</div><div class="value" id="coords">-</div></div>
      <div class="card"><div class="label">Uydu Sayisi</div><div class="value" id="sats">-</div></div>
      <div class="card"><div class="label">Rakim (m)</div><div class="value" id="alt">-</div></div>
      <div class="card"><div class="label">Hiz (km/h)</div><div class="value" id="speed">-</div></div>
      <div class="card"><div class="label">Son Guncelleme (UTC)</div><div class="value" id="time">-</div></div>
    </div>
  </div>

  <div class="bottom-row">
    <div class="bottom-card">
      <h2>LiDAR - Hedef Konum</h2>
      <div class="mini-value" id="lidarDist">-</div>
      <div class="label" id="lidarStatus" style="margin-top:6px;">-</div>
      <div class="label" style="margin-top:10px;">Hedef GPS Koordinati</div>
      <div class="mini-value" id="targetCoords" style="font-size:15px;">-</div>
      <div class="label" style="margin-top:6px;">Operator-Hedef Mesafesi</div>
      <div class="mini-value" id="targetDist" style="font-size:15px;">-</div>
      <button id="markBtn" onclick="markTarget()" style="margin-top:12px; width:100%; padding:12px; font-size:15px; font-weight:bold; background:#4CAF50; color:#111; border:none; border-radius:8px; cursor:pointer;">Hedefi Isaretle</button>
      <div class="label" id="markStatus" style="margin-top:6px;"></div>
      <button id="resetBtn" onclick="resetMarkers()" style="margin-top:8px; width:100%; padding:10px; font-size:13px; font-weight:bold; background:#e74c3c; color:#111; border:none; border-radius:8px; cursor:pointer;">Tum Isaretleri Sifirla</button>
      <div class="label" style="margin-top:14px;">Isaretlenmis Hedefler</div>
      <div id="markersList" style="margin-top:6px; font-size:13px; max-height:150px; overflow-y:auto;"></div>
    </div>

    <div class="bottom-card">
      <h2>IMU (Roll / Pitch / Yaw)</h2>
      <div class="mini-value" id="imuVals">-</div>
      <div class="label" id="imuStatus" style="margin-top:6px;">-</div>
    </div>
  </div>

<script>
let map = L.map('map').setView([41.0, 29.0], 15);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  maxZoom: 19,
  attribution: 'OpenStreetMap'
}).addTo(map);

let operatorMarker = null;
let targetMarkers = [];
let mapCentered = false;

const operatorIcon = L.divIcon({
  className: '',
  html: '<div style="background:#4CAF50; width:18px; height:18px; border-radius:50%; border:3px solid #fff;"></div>',
  iconSize: [18, 18],
  iconAnchor: [9, 9]
});

const targetIcon = L.divIcon({
  className: '',
  html: '<div style="background:#e74c3c; width:16px; height:16px; border-radius:50%; border:2px solid #fff;"></div>',
  iconSize: [16, 16],
  iconAnchor: [8, 8]
});

const updateOperatorMarker = function(lat, lng) {
  if (!operatorMarker) {
    operatorMarker = L.marker([lat, lng], {icon: operatorIcon}).addTo(map).bindPopup('Operator');
  } else {
    operatorMarker.setLatLng([lat, lng]);
  }
  if (!mapCentered) {
    map.setView([lat, lng], 18);
    mapCentered = true;
  }
};

const redrawTargetMarkers = function(markersArr) {
  targetMarkers.forEach(function(m) { map.removeLayer(m); });
  targetMarkers = [];
  markersArr.forEach(function(m) {
    const marker = L.marker([m.lat, m.lng], {icon: targetIcon}).addTo(map)
      .bindPopup('Hedef #' + m.id + '<br>' + m.lat.toFixed(6) + ', ' + m.lng.toFixed(6) + '<br>Mesafe: ' + m.distance.toFixed(1) + ' m');
    targetMarkers.push(marker);
  });
};

const updateGPS = function() {
  fetch('/data/gps').then(function(res) { return res.json(); }).then(function(d) {
    document.getElementById('fixStatus').innerHTML = d.fix
      ? '<span class="status-ok">FIX VAR</span>' : '<span class="status-bad">FIX BEKLENIYOR...</span>';
    document.getElementById('coords').innerText = d.lat.toFixed(6) + ', ' + d.lng.toFixed(6);
    document.getElementById('sats').innerText = d.sats;
    document.getElementById('alt').innerText = d.alt.toFixed(1) + ' m';
    document.getElementById('speed').innerText = d.speed.toFixed(1) + ' km/h';
    document.getElementById('time').innerText = d.time;
    if (d.fix) {
      updateOperatorMarker(d.lat, d.lng);
    }
  });
};

const updateLidar = function() {
  fetch('/data/lidar').then(function(res) { return res.json(); }).then(function(d) {
    document.getElementById('lidarDist').innerText = d.ok ? d.distance.toFixed(1) + ' cm' : '-';
    document.getElementById('lidarStatus').innerHTML = d.ok
      ? '<span class="status-ok">OK</span>' : '<span class="status-bad">VERI YOK</span>';
    if (d.targetValid) {
      document.getElementById('targetCoords').innerText = d.targetLat.toFixed(6) + ', ' + d.targetLng.toFixed(6);
      document.getElementById('targetDist').innerText = d.distanceToTarget.toFixed(1) + ' m';
    } else {
      document.getElementById('targetCoords').innerText = 'Hesaplanamadi';
      document.getElementById('targetDist').innerText = '-';
    }
  });
};

const updateIMU = function() {
  fetch('/data/imu').then(function(res) { return res.json(); }).then(function(d) {
    document.getElementById('imuVals').innerText = d.ok
      ? d.roll.toFixed(1) + ' / ' + d.pitch.toFixed(1) + ' / ' + d.yaw.toFixed(1) : '-';
    document.getElementById('imuStatus').innerHTML = d.ok
      ? '<span class="status-ok">OK</span>' : '<span class="status-bad">VERI YOK</span>';
  });
};

const markTarget = function() {
  fetch('/mark').then(function(res) { return res.json(); }).then(function(d) {
    const statusEl = document.getElementById('markStatus');
    if (d.success) {
      statusEl.innerHTML = '<span class="status-ok">Hedef #' + d.count + ' isaretlendi</span>';
      loadMarkers();
    } else {
      statusEl.innerHTML = '<span class="status-bad">' + d.error + '</span>';
    }
  });
};

const resetMarkers = function() {
  if (!confirm('Tum isaretlenmis hedefler silinecek. Emin misiniz?')) return;
  fetch('/reset_markers').then(function(res) { return res.json(); }).then(function(d) {
    if (d.success) {
      document.getElementById('markStatus').innerHTML = '<span class="status-ok">Tum isaretler silindi</span>';
      loadMarkers();
    }
  });
};

const loadMarkers = function() {
  fetch('/data/markers').then(function(res) { return res.json(); }).then(function(d) {
    const listEl = document.getElementById('markersList');
    if (d.markers.length === 0) {
      listEl.innerText = 'Henuz isaretlenmis hedef yok.';
    } else {
      let html = '';
      d.markers.forEach(function(m) {
        html += '<div style="padding:4px 0; border-bottom:1px solid #333;">';
        html += '#' + m.id + ': ' + m.lat.toFixed(6) + ', ' + m.lng.toFixed(6) + ' (' + m.distance.toFixed(1) + ' m)';
        html += '</div>';
      });
      listEl.innerHTML = html;
    }
    redrawTargetMarkers(d.markers);
  });
};

const updateAll = function() {
  updateGPS();
  updateLidar();
  updateIMU();
};

setInterval(updateAll, 2000);
updateAll();
loadMarkers();
</script>
</body>
</html>
)WEBGUI2726";

  server.send(200, "text/html", html);
}

// ================== TERMAL SAYFASI (AYRI ROUTE) ==================
void handleThermalPage() {
  String html = R"THERMALPAGE(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Termal Kamera</title>
<style>
  body { font-family: Arial, sans-serif; background:#1a1a1a; color:#eee; margin:0; padding:15px; }
  h1 { color:#e67e22; font-size:18px; margin:0 0 12px 0; }
  a.back-link {
    display:inline-block; background:#4CAF50; color:#111; text-decoration:none;
    padding:8px 14px; border-radius:8px; font-weight:bold; font-size:13px; margin-bottom:15px;
  }
  .status { margin-bottom:10px; font-size:13px; }
  .status-ok { color:#4CAF50; }
  .status-bad { color:#e74c3c; }
  #thermalCanvas { background:#111; border-radius:10px; display:block; }
</style>
</head>
<body>
  <h1>Termal Kamera (AMG8833)</h1>
  <a class="back-link" href="/">Ana Sayfaya Don</a>
  <div class="status" id="thermalStatus">-</div>
  <canvas id="thermalCanvas" width="240" height="240"></canvas>

<script>
const canvas = document.getElementById('thermalCanvas');
const ctx = canvas.getContext('2d');
const SRC_GRID = 8;
const INT_GRID = 24;
const OUTPUT_SIZE = 240;
const CELL = OUTPUT_SIZE / INT_GRID;

const MINTEMP = 27;
const MAXTEMP = 32;

const tempToColor = function(t, minT, maxT) {
  const ratio = Math.max(0, Math.min(1, (t - minT) / (maxT - minT)));
  let r, g, b;
  if (ratio < 0.25) {
    const lr = ratio / 0.25;
    r = 0; g = Math.round(255 * lr); b = 255;
  } else if (ratio < 0.5) {
    const lr = (ratio - 0.25) / 0.25;
    r = 0; g = 255; b = Math.round(255 * (1 - lr));
  } else if (ratio < 0.75) {
    const lr = (ratio - 0.5) / 0.25;
    r = Math.round(255 * lr); g = 255; b = 0;
  } else {
    const lr = (ratio - 0.75) / 0.25;
    r = 255; g = Math.round(255 * (1 - lr)); b = 0;
  }
  return [r, g, b];
};

// ---- Bicubic interpolasyon (M5Stack ornegindeki C koddan birebir uyarlandi) ----
function getPoint(p, rows, cols, x, y) {
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= cols) x = cols - 1;
  if (y >= rows) y = rows - 1;
  return p[y * cols + x];
}

function cubicInterpolate(p, x) {
  return p[1] + 0.5 * x * (p[2] - p[0] + x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] + x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
}

function bicubicInterpolate(p, x, y) {
  const arr = [0, 0, 0, 0];
  arr[0] = cubicInterpolate(p.slice(0, 4), x);
  arr[1] = cubicInterpolate(p.slice(4, 8), x);
  arr[2] = cubicInterpolate(p.slice(8, 12), x);
  arr[3] = cubicInterpolate(p.slice(12, 16), x);
  return cubicInterpolate(arr, y);
}

function getAdjacents2D(src, rows, cols, x, y) {
  const dest = new Array(16);
  for (let dy = -1; dy < 3; dy++) {
    for (let dx = -1; dx < 3; dx++) {
      dest[(dy + 1) * 4 + (dx + 1)] = getPoint(src, rows, cols, x + dx, y + dy);
    }
  }
  return dest;
}

function interpolateImage(src, srcRows, srcCols, destRows, destCols) {
  const dest = new Array(destRows * destCols);
  const muX = (srcCols - 1.0) / (destCols - 1.0);
  const muY = (srcRows - 1.0) / (destRows - 1.0);

  for (let yIdx = 0; yIdx < destRows; yIdx++) {
    for (let xIdx = 0; xIdx < destCols; xIdx++) {
      const x = xIdx * muX;
      const y = yIdx * muY;
      const adj2d = getAdjacents2D(src, srcRows, srcCols, Math.floor(x), Math.floor(y));
      const fracX = x - Math.floor(x);
      const fracY = y - Math.floor(y);
      dest[yIdx * destCols + xIdx] = bicubicInterpolate(adj2d, fracX, fracY);
    }
  }
  return dest;
}

const drawThermal = function(pixels) {
  const actualMin = Math.min.apply(null, pixels);
  const actualMax = Math.max.apply(null, pixels);

  const interpolated = interpolateImage(pixels, SRC_GRID, SRC_GRID, INT_GRID, INT_GRID);

  for (let y = 0; y < INT_GRID; y++) {
    for (let x = 0; x < INT_GRID; x++) {
      const val = interpolated[y * INT_GRID + x];
      const rgb = tempToColor(val, MINTEMP, MAXTEMP);
      ctx.fillStyle = 'rgb(' + rgb.join(',') + ')';
      ctx.fillRect(x * CELL, y * CELL, CELL, CELL);
    }
  }

  ctx.fillStyle = 'white';
  ctx.font = 'bold 14px Arial';
  ctx.fillText('Max: ' + actualMax.toFixed(1) + ' C', 10, 20);
  ctx.fillText('Min: ' + actualMin.toFixed(1) + ' C', 10, 38);
};

const updateThermal = function() {
  fetch('/data/thermal').then(function(res) { return res.json(); }).then(function(d) {
    if (d.ok) {
      document.getElementById('thermalStatus').innerHTML = '<span class="status-ok">VERI ALINIYOR</span>';
      drawThermal(d.pixels);
    } else {
      document.getElementById('thermalStatus').innerHTML = '<span class="status-bad">KAMERA HAZIR DEGIL</span>';
    }
  }).catch(function() {
    document.getElementById('thermalStatus').innerHTML = '<span class="status-bad">BAGLANTI HATASI</span>';
  });
};

setInterval(updateThermal, 1000);
updateThermal();
</script>
</body>
</html>
)THERMALPAGE";

  server.send(200, "text/html", html);
}