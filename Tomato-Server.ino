#include <WiFiS3.h>

const char* ssid = "Madeline's Phone";  // or whatever your network is called
const char* password = "password here";

const char* host = "database link here";
const char* auth = ""; // leave empty if using open rules

WiFiSSLClient client;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("\nWiFi connected");
}

void loop() {
  if (!client.connect(host, 443)) {
    Serial.println("Connection failed");
    return;
  }

  int soilMoisture = 42;
  // Construct JSON manually
  String data = "{";
  data += "\"soilMoisture\":" + String(soilMoisture);
  data += "}";
  // Firebase URL path
  String url = "/plantData.json";

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
  delay(10000);
}
