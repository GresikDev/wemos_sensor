#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
/* #include <DHT.h> */
#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

double ta = 0; /* Air Temperature */
double ht = 0; /* Humidity */
double tw = 0; /* Water Temperature */
double bl = 0; /* Battery Level */
double ph = 0; /* Acidity */
double ec = 0; /* Electric Conductivity */

const char* ssid = "SIHIPO"; /* wifi ssid */
const char* password = "sistemhidroponik"; /* wifi psk */
const char* ap_ssid = "SIHIPO_AP"; /* AP ssid */
const char* ap_password = "sistemhidroponik"; /* AP psk */
const String device_id = "S01";
const String device_type = "SIHIPO_S";

ESP8266WebServer server(80);

DHTesp dht;
OneWire oneWire(D1);
DallasTemperature sensors(&oneWire);

void handleRoot() {
  delay(dht.getMinimumSamplingPeriod());
  ta = dht.getTemperature();
  ht = dht.getHumidity();
  delay(1);
  sensors.requestTemperatures();
  tw = sensors.getTempCByIndex(0);
  delay(1);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
  delay(1);
  ec = (double) analogRead(A0) * (25.0 / tw) * 0.002;
  delay(1);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
  delay(1);
  ph = (double) analogRead(A0) / 50.0;
  delay(1);
  digitalWrite(D5, LOW);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
  delay(1);
  bl = ((double) analogRead(A0) / 1023.0) * 100.0;

  String out = "{\"id\":\"" + device_id + "\",\"type\":\"" + device_type + "\",\"value\":{";
  String out2 = "";
  if (!isnan(ph) && (ph > 0)) out2 += " \"PH\":\"" + String(ph) + "\"";
  if (!isnan(ec) && (tw > -127) && (ec >= 0.01)) out2 += " \"EC\":\"" + String(ec) + "\"";
  if (!isnan(ta)) out2 += " \"TA\":\"" + String(ta) + "\"";
  if (!isnan(tw) && (tw > -127)) out2 += " \"TW\":\"" + String(tw) + "\"";
  if (!isnan(ht)) out2 += " \"HT\":\"" + String(ht) + "\"";
  if (!isnan(bl) && (bl > 0)) out2 += " \"BL\":\"" + String(bl) + "\"";
  out2.trim();
  out2.replace(" ", ",");

  out += out2;
  out += "}}";
  server.send(200, "application/json", out);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);

  if (digitalRead(D0) == HIGH) {
    Serial.println();
    Serial.print("Configuring access point...");
    WiFi.softAP(ap_ssid, ap_password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  } else {
    WiFi.begin(ssid, password);
    Serial.println("");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  pinMode(A0, INPUT);
  pinMode(D0, INPUT);
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
  pinMode(D3, INPUT_PULLUP);
  pinMode(D4, INPUT_PULLUP);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  sensors.begin();
  dht.setup(D2, DHTesp::DHT11);

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
