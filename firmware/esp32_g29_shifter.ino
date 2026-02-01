#define USB_VID 0x046D
#define USB_PID 0xC24F

#include "USB.h"
#include "USBHID.h"
#include "USBHIDGamepad.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ==============================
// Global Variables
// ==============================
String lastGear = "";
bool lastRev = false;
float xSmooth = 0;
float ySmooth = 0;
const float alpha = 0.3;

// ==============================
// Pins
// ==============================
#define X_AXIS_PIN   1
#define Y_AXIS_PIN   2
#define REVERSE_PIN  3

// ==============================
// HID & Web
// ==============================
USBHIDGamepad Gamepad;
WebServer server(80);
Preferences prefs;

// ==============================
// WiFi AP
// ==============================
const char* ssid = "G29-Shifter";
const char* password = "g29config";

// ==============================
// HID Button Map
// ==============================
#define BTN_1   0
#define BTN_2   1
#define BTN_3   2
#define BTN_4   3
#define BTN_5   4
#define BTN_6   5
#define BTN_REV 6

// ==============================
// Gear Zones
// ==============================
struct GearZone {
  int xmin, xmax, ymin, ymax;
};

GearZone gears[7];

// ==============================
// Defaults
// ==============================
void loadDefaults() {
  gears[0] = {1000, 2000, 3900, 4096}; // Gear 1
  gears[1] = {1000, 2000, 0,    650};  // Gear 2
  gears[2] = {2000, 3500, 3900, 4096}; // Gear 3
  gears[3] = {2000, 3500, 0,    650};  // Gear 4
  gears[4] = {3500, 4096, 3900, 4096}; // Gear 5
  gears[5] = {3500, 4096, 0,    650};  // Gear 6
  gears[6] = {3500, 4096, 300,  650};  // Reverse
}

// ==============================
// Load / Save Settings
// ==============================
void loadSettings() {
  prefs.begin("shifter", true);
  if (!prefs.getBool("init", false)) {
    loadDefaults();
  } else {
    for (int i = 0; i < 7; i++) {
      gears[i].xmin = prefs.getInt(("g" + String(i) + "x1").c_str());
      gears[i].xmax = prefs.getInt(("g" + String(i) + "x2").c_str());
      gears[i].ymin = prefs.getInt(("g" + String(i) + "y1").c_str());
      gears[i].ymax = prefs.getInt(("g" + String(i) + "y2").c_str());
    }
  }
  prefs.end();
}

void saveSettings() {
  prefs.begin("shifter", false);
  prefs.putBool("init", true);
  for (int i = 0; i < 7; i++) {
    prefs.putInt(("g" + String(i) + "x1").c_str(), gears[i].xmin);
    prefs.putInt(("g" + String(i) + "x2").c_str(), gears[i].xmax);
    prefs.putInt(("g" + String(i) + "y1").c_str(), gears[i].ymin);
    prefs.putInt(("g" + String(i) + "y2").c_str(), gears[i].ymax);
  }
  prefs.end();
}

// ==============================
// Gear Detection
// ==============================
String detectGear(int x, int y, bool revPressed) {
  if (revPressed && x >= gears[6].xmin && x <= gears[6].xmax &&
      y >= gears[6].ymin && y <= gears[6].ymax) return "R";

  if (x >= gears[4].xmin && x <= gears[4].xmax &&
      y >= gears[4].ymin && y <= gears[4].ymax) return "5";

  if (!revPressed && x >= gears[5].xmin && x <= gears[5].xmax &&
      y >= gears[5].ymin && y <= gears[5].ymax) return "6";

  for (int i = 0; i < 4; i++) {
    if (x >= gears[i].xmin && x <= gears[i].xmax &&
        y >= gears[i].ymin && y <= gears[i].ymax) return String(i + 1);
  }

  return "N"; // Neutral
}

// ==============================
// Serial Debug
// ==============================
void printSerialStatus(int x, int y, bool rev, const String& gear) {
  if (gear != lastGear || rev != lastRev) {
    Serial.print("X="); Serial.print(x);
    Serial.print(" Y="); Serial.print(y);
    Serial.print(" REV="); Serial.print(rev ? "PRESSED" : "RELEASED");
    Serial.print(" GEAR="); Serial.println(gear);

    lastGear = gear;
    lastRev = rev;
  }
}

// ==============================
// Web Handlers
// ==============================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>G29 Shifter Calibration</title>
<style>
body { font-family: 'Segoe UI', Tahoma, sans-serif; background:#121212; color:#f5f5f5; margin:0; padding:0; }
header { background:#1f1f1f; padding:1rem; text-align:center; font-size:1.5rem; font-weight:bold; }
.container { padding:1rem; max-width:600px; margin:auto; }
.live { display:flex; justify-content:space-around; margin:1rem 0; padding:1rem; background:#1f1f1f; border-radius:8px; }
.live div { text-align:center; }
form { background:#1f1f1f; padding:1rem; border-radius:8px; }
input { width:100%; padding:0.5rem; margin:0.3rem 0 1rem 0; border-radius:4px; border:none; }
button { background:#4CAF50; color:white; padding:0.7rem 1.2rem; border:none; border-radius:6px; font-size:1rem; cursor:pointer; }
button:hover { background:#45a049; }
.gear-section { margin-bottom:1rem; padding:0.5rem; background:#2c2c2c; border-radius:6px; }
.gear-section h4 { margin:0 0 0.5rem 0; }
</style>
</head>
<body>
<header>G29 Shifter Calibration</header>
<div class="container">
<div class="live">
  <div>X<br><span id="xval">---</span></div>
  <div>Y<br><span id="yval">---</span></div>
  <div>Rev<br><span id="revval">---</span></div>
  <div>Gear<br><span id="gearval">---</span></div>
</div>
<form action="/save">
)rawliteral";

  // Generate gear inputs dynamically
  for (int i = 0; i < 7; i++) {
    html += "<div class='gear-section'>";
    html += "<h4>Gear " + String(i + 1) + (i == 6 ? " (Reverse)" : "") + "</h4>";
    html += "Xmin <input name='g" + String(i) + "x1' value='" + String(gears[i].xmin) + "'><br>";
    html += "Xmax <input name='g" + String(i) + "x2' value='" + String(gears[i].xmax) + "'><br>";
    html += "Ymin <input name='g" + String(i) + "y1' value='" + String(gears[i].ymin) + "'><br>";
    html += "Ymax <input name='g" + String(i) + "y2' value='" + String(gears[i].ymax) + "'><br>";
    html += "</div>";
  }

  html += R"rawliteral(
<button type="submit">Save to Flash</button>
</form>
<script>
function updateLive() {
  fetch('/live')
    .then(r => r.json())
    .then(d => {
      document.getElementById('xval').textContent = d.x;
      document.getElementById('yval').textContent = d.y;
      document.getElementById('revval').textContent = d.rev ? "PRESSED" : "RELEASED";
      const gearEl = document.getElementById('gearval');
      gearEl.textContent = d.gear;
      gearEl.style.color = (d.gear === "R") ? "red" : (d.gear === "N") ? "gray" : "lightgreen";
    });
}
setInterval(updateLive, 100);
</script>
</div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleSave() {
  for (int i = 0; i < 7; i++) {
    gears[i].xmin = server.arg("g" + String(i) + "x1").toInt();
    gears[i].xmax = server.arg("g" + String(i) + "x2").toInt();
    gears[i].ymin = server.arg("g" + String(i) + "y1").toInt();
    gears[i].ymax = server.arg("g" + String(i) + "y2").toInt();
  }
  saveSettings();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleReset() {
  prefs.begin("shifter", false);
  prefs.clear();
  prefs.end();
  loadDefaults();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLive() {
  int x = analogRead(X_AXIS_PIN);
  int y = analogRead(Y_AXIS_PIN);
  bool rev = digitalRead(REVERSE_PIN);
  String gear = detectGear(x, y, rev);

  String json = "{";
  json += "\"x\":" + String(x) + ",";
  json += "\"y\":" + String(y) + ",";
  json += "\"rev\":" + String(rev) + ",";
  json += "\"gear\":\"" + gear + "\"}";
  server.send(200, "application/json", json);
}

// ==============================
// Setup
// ==============================
void setup() {
  pinMode(REVERSE_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(X_AXIS_PIN, ADC_11db);
  analogSetPinAttenuation(Y_AXIS_PIN, ADC_11db);

  USB.manufacturerName("Logitech");
  USB.productName("Logitech G29 Shifter");
  USB.begin();
  Gamepad.begin();

  loadSettings();

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/reset", handleReset);
  server.on("/live", handleLive);
  server.begin();
}

// ==============================
// Main Loop
// ==============================
void loop() {
  server.handleClient();

  int xRaw = analogRead(X_AXIS_PIN);
  int yRaw = analogRead(Y_AXIS_PIN);
  bool revPressed = (digitalRead(REVERSE_PIN) == HIGH);

  xSmooth = alpha * xRaw + (1 - alpha) * xSmooth;
  ySmooth = alpha * yRaw + (1 - alpha) * ySmooth;

  int x = (int)xSmooth;
  int y = (int)ySmooth;
  String gear = detectGear(x, y, revPressed);

  printSerialStatus(x, y, revPressed, gear);

  uint32_t buttons = 0;
  if (gear == "1") buttons |= (1 << BTN_1);
  else if (gear == "2") buttons |= (1 << BTN_2);
  else if (gear == "3") buttons |= (1 << BTN_3);
  else if (gear == "4") buttons |= (1 << BTN_4);
  else if (gear == "5") buttons |= (1 << BTN_5);
  else if (gear == "6") buttons |= (1 << BTN_6);
  else if (gear == "R") buttons |= (1 << BTN_REV);

  Gamepad.send(0, 0, 0, 0, 0, 0, 0x08, buttons);

  delay(10);
}
