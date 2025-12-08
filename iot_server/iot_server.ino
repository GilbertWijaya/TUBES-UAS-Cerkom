#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define SDA_PIN 26
#define SCL_PIN 27
#define LED_BIRU 33
#define LED_MERAH 25
#define BUZZER 14

// === WIFI ===
const char* ssid = "Redmi Note 13";
const char* pass = "indomie123";

// === SERVER ===
WebServer server(80);

// === LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === VARIABEL DATA ===
int totalTelur = 0;
float conf = 0.0;

// =============================
//  HANDLE DATA POST JSON
// =============================
void handleData() {

  // === CORS WAJIB ===
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request: No JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.println("JSON diterima:");
  Serial.println(body);

  // parsing JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "text/plain", "JSON Parse Error");
    return;
  }

  bool detected = doc["detected"];
  int count     = doc["count"];
  float confVal = doc["confidence"];

  totalTelur = count;
  conf = confVal;

  // LED & Buzzer
  if (detected) {
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_BIRU, HIGH);
    tone(BUZZER, 900, 150);
  } else {
    digitalWrite(LED_BIRU, LOW);
    digitalWrite(LED_MERAH, HIGH);
  }

  // LCD update
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Telur: ");
  lcd.print(totalTelur);

  lcd.setCursor(0, 1);
  lcd.print("Conf: ");
  lcd.print(conf, 2);

  server.send(200, "text/plain", "OK");
}


// =============================
//   SETUP
// =============================
void setup() {
  Serial.begin(74880);

  pinMode(LED_BIRU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_BIRU, LOW);
  digitalWrite(LED_MERAH, HIGH);

  // WiFi
  WiFi.begin(ssid, pass);
  Serial.println("Menghubungkan WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(800);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  // LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.print("Menunggu Data...");

  // =============================
  //  FIX: CORS preflight OPTIONS
  // =============================
  server.on("/data", HTTP_OPTIONS, [](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(200);
  });

  // =============================
  //  FIX: POST handler with body
  // =============================
  server.on("/data", HTTP_POST, [](){
    handleData();
  });

  server.begin();
}


// =============================
void loop() {
  server.handleClient();
}
