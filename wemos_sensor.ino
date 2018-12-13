#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
/* #include <DHT.h> */
#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0; // ,tdsValue = 0,temperature = 25;

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

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

void analogSwitch(byte a) {
  digitalWrite(D5, a & 1);
  digitalWrite(D6, a & 2);
  digitalWrite(D7, a & 4);
  digitalWrite(D8, a & 8);
}

void handleRoot() {
  delay(dht.getMinimumSamplingPeriod());
  ta = dht.getTemperature();
  ht = dht.getHumidity();
  delay(1);
  sensors.requestTemperatures();
  tw = sensors.getTempCByIndex(0);
  delay(1);
  analogSwitch(0);
  delay(1);
  bl = ((double) analogRead(A0) / 1024.0) * 100.0;
  delay(1);
  analogSwitch(1);
  delay(1);

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(A0);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (tw - 25.0);
    float compensationVolatge = averageVoltage / compensationCoefficient;
    ec = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;
  }

  delay(1);
  analogSwitch(2);
  delay(1);
  ph = (double) analogRead(A0) / 50.0;

  String out = "{\"id\":\"" + device_id + "\",\"type\":\"" + device_type + "\",\"value\":{";
  String out2 = "";
  if (!isnan(ph) && (ph >= 0.01)) out2 += " \"PH\":\"" + String(ph) + "\"";
  if (!isnan(ec) && (tw > -127) && (ec >= 0.01)) out2 += " \"EC\":\"" + String(ec) + "\"";
  if (!isnan(ta)) out2 += " \"TA\":\"" + String(ta) + "\"";
  if (!isnan(tw) && (tw > -127)) out2 += " \"TW\":\"" + String(tw) + "\"";
  if (!isnan(ht)) out2 += " \"HT\":\"" + String(ht) + "\"";
  if (!isnan(bl) && (bl >= 0.01)) out2 += " \"BL\":\"" + String(bl) + "\"";
  out2.trim();
  out2.replace(" ", ",");

  out += out2;
  out += "}}";
  server.send(200, "application/json", out);
}

void handleUI() {
  String out = "";
  out += "<html>";
  out += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  out += "  <body>";
  out += "    <pre>";
  out += "Keasaman      (PH) : <span id=\"PH\"></span><br/>";
  out += "Konduktivitas (EC) : <span id=\"EC\"></span><br/>";
  out += "Suhu Air      (TW) : <span id=\"TW\"></span><br/>";
  out += "Suhu Udara    (TA) : <span id=\"TA\"></span><br/>";
  out += "Kelembapan    (HT) : <span id=\"HT\"></span><br/>";
  out += "Baterai       (BL) : <span id=\"BL\"></span><br/>";
  out += "    </pre>";
  out += "    <script>";
  out += "      function reloadSensor() {";
  out += "        var xhttp = new XMLHttpRequest();";
  out += "        xhttp.onreadystatechange = function() {";
  out += "          if (this.readyState == 4 && this.status == 200) {";
  out += "            var datas = JSON.parse(this.responseText);";
  out += "            for (var key in datas.value) {";
  out += "              if (datas.value.hasOwnProperty(key)) {";
  out += "                document.getElementById(key).innerHTML = datas.value[key];";
  out += "              }";
  out += "            }";
  out += "            setTimeout(reloadSensor, 1000);";
  out += "          }";
  out += "        };";
  out += "        xhttp.open(\"GET\", \"/\", true);";
  out += "        xhttp.send();";
  out += "      }";
  out += "      reloadSensor();";
  out += "    </script>";
  out += "  </body>";
  out += "</html>";
  server.send(200, "text/html", out);
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
  server.on("/ui", handleUI);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
