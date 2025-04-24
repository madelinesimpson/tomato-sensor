// === Original Libraries ===
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <SPI.h>

// === WiFi & Firebase Upload Libraries ===
#include <WiFiS3.h>

const char* ssid = "Madeline's Phone";
const char* password = "password here";
const char* host = "your-firebase-database-url"; // e.g. "your-project.firebaseio.com"
WiFiSSLClient client;

// === OLED Display Setup ===
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8
Adafruit_ST7796S display(TFT_CS, TFT_DC, TFT_RST);
uint8_t rotate = 0;
#define CORNER_RADIUS 0

// === Sensor and Joystick Pin Setup ===
#define sensorPower 7
#define sensorPin A3
int joyPin1 = A0;
int joyPin2 = A1;
const int joyButton = 2;

// === Moisture Range for Each Plant ===
int idealMin[] = {35, 20, 40, 32};
int idealMax[] = {45, 26, 47, 43};

int selectedLabel = 0;
unsigned long lastMoveTime = 0;
const int moveDelay = 300;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  display.init(320, 480, 0, 0, ST7796S_RGB);
  display.setRotation(1);
  display.fillScreen(0);
  SPI.begin();

  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW);
  delay(500);
  drawStaticMoistureBox(selectedLabel);

  // === WiFi Connect ===
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
}

void loop() {
  updateMoistureBar(selectedLabel);
  delay(500);

  Serial.print("Analog output: ");
  Serial.println(readSensor());
  delay(500);

  int joyX = analogRead(joyPin1);
  unsigned long now = millis();

  if (now - lastMoveTime > moveDelay) {
    if (joyX < 400) {
      selectedLabel = (selectedLabel + 3) % 4;
      lastMoveTime = now;
      drawStaticMoistureBox(selectedLabel);
    } else if (joyX > 600) {
      selectedLabel = (selectedLabel + 1) % 4;
      lastMoveTime = now;
      Serial.println(selectedLabel);
      drawStaticMoistureBox(selectedLabel);
    }
  }

  delay(100);
}

int readSensor() {
  digitalWrite(sensorPower, HIGH);
  delay(10);
  int val = analogRead(sensorPin);
  digitalWrite(sensorPower, LOW);
  return val;
}

void sendSoilMoistureToFirebase(int moisturePercent) {
  if (!client.connect(host, 443)) {
    Serial.println("Connection to Firebase failed");
    return;
  }

  String data = "{\"soilMoisture\":" + String(moisturePercent) + "}";

  String url = "/plantData.json"; // Change this to your Firebase path
  client.println("PUT " + url + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.println(data);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  String line = client.readStringUntil('\n');
  Serial.println("Firebase response:");
  Serial.println(line);
  client.stop();
}

void drawStaticMoistureBox(int selectedLabel) {
  display.setFont();
  display.setTextSize(3);
  display.setTextColor(0xFFFF);

  int boxW = 220;
  int boxH = 80;
  int x = (display.width() - boxW) / 2;
  int y = (display.height() / 2) - boxH - 20;

  for (int i = 0; i < 3; i++) {
    display.drawRect(x - i, y - i, boxW + 2 * i, boxH + 2 * i, 0xFFFF);
  }

  display.setCursor(x - 45, y + boxH / 2 - 12);
  display.print("M");

  display.setTextSize(2);
  int labelBoxW = 150;
  int labelBoxH = 40;
  int padding = 10;

  int col1X = (display.width() / 2) - labelBoxW - padding;
  int col2X = (display.width() / 2) + padding;
  int row1Y = display.height() - 150;
  int row2Y = display.height() - 105;

  struct Label {
    const char* text;
    int x;
    int y;
  } labels[] = {
    {"TOMATO", col1X, row1Y},
    {"JALAPENO", col2X, row1Y},
    {"CILANTRO", col1X, row2Y},
    {"BELL PEPPER", col2X, row2Y}
  };

  display.fillRect(0, row1Y - 4, display.width(), labelBoxH * 2 + 20, 0x0000);

  for (int i = 0; i < 4; i++) {
    if (i == selectedLabel) {
      for (int t = 0; t < 3; t++) {
        display.drawRect(labels[i].x - 4 - t, labels[i].y - t,
                         labelBoxW + 8 + t * 2, labelBoxH + 8 + t * 2, 0x07C0);
      }
    }

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(labels[i].text, labels[i].x, labels[i].y, &x1, &y1, &w, &h);
    int centerX = labels[i].x + (labelBoxW - w) / 2;
    int centerY = labels[i].y + (labelBoxH + h) / 2 - 4;
    display.setCursor(centerX, centerY);
    display.setTextColor(0xFFFF);
    display.print(labels[i].text);
  }
}

void updateMoistureBar(int selectedLabel) {
  int boxW = 220;
  int boxH = 80;
  int x = (display.width() - boxW) / 2;
  int y = (display.height() / 2) - boxH - 20;

  display.fillRect(x + 2, y + 2, boxW - 4, boxH - 4, 0x0000);

  int raw = readSensor();

  int fillWidth;
  int moisturePercent;
  if (raw <= 575) {
    moisturePercent = 100;
    fillWidth = 220;
  } else if (raw >= 1050) {
    moisturePercent = 0;
    fillWidth = 0;
  } else {
    moisturePercent = abs(((1050 - raw) * 100) / (1050 - 575));
    fillWidth = (boxW * moisturePercent) / 100;
  }

  display.fillRect(x + 2, y + 2, fillWidth - 4, boxH - 4, 0x07C0);

  int outlineStartPercent = idealMin[selectedLabel];
  int outlineEndPercent = idealMax[selectedLabel];
  int outlineX = x + 2 + (boxW * outlineStartPercent) / 100;
  int outlineW = ((boxW * outlineEndPercent) / 100) - ((boxW * outlineStartPercent) / 100);

  for (int i = 0; i < 3; i++) {
    display.drawRect(outlineX - i, y + 2 - i, outlineW + 2 * i, boxH - 4 + 2 * i, 0xFFFF);
  }

  // === Send moisture to Firebase ===
  sendSoilMoistureToFirebase(moisturePercent);
}
