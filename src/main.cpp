#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

// --- Terminal Colors ---
#define CLR_RST "\x1B[0m"
#define CLR_RED "\x1B[31m"
#define CLR_GRN "\x1B[32m"
#define CLR_YLW "\x1B[33m"
#define CLR_BLU "\x1B[34m"
#define CLR_CYN "\x1B[36m"

// --- CẤU HÌNH ---
const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *mqtt_server = "*";
const char *mqtt_user = "*";
const char *mqtt_pass = "*";
const int mqtt_port = 8883;

// --- NTP SERVER ---
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // UTC+7 (Việt Nam)
const int daylightOffset_sec = 0;

// --- CHÂN THIẾT BỊ ---
#define DHTPIN 15
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define LDR_PIN 35
#define PUMP_LED 16
#define LIGHT_LED 18
#define BUZZER 19
#define SERVO_PIN 17

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

DHT dht(DHTPIN, DHTTYPE);
Servo roofServo;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Biến điều khiển config (từ AUTO mode)
String currentCrop = "Waiting...";
int minMoisture = 0, maxMoisture = 0;
String currentMode = "WAIT";

// Trạng thái thiết bị hiện tại
bool isPumpOn = false;
bool isLightOn = false;
int roofAngle = 0; // 0 = đóng, 90 = mở

// Biến lưu trạng thái cũ để so sánh thay đổi và chống tràn terminal
float lastSentT = -1.0, lastSentH = -1.0;
int lastSentSoil = -1, lastSentLdr = -1;
bool lastSentPump = false, lastSentLight = false;
int lastSentRoof = -1;
String lastSentAlert = "";

unsigned long lastSend = 0;

// Hàm lọc nhiễu Analog
int readAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 10;
}

void setup_display() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Loi OLED!");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Smart Farm Starting...");
  display.display();
}

void update_oled(float t, float h, int s, int ldr_val, struct tm timeinfo) {
  display.clearDisplay();
  display.setCursor(0, 0);

  // Hiển thị giờ hiện tại
  display.printf("Time: %02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);
  display.printf("Mode: %s | %s\n", currentMode.c_str(), currentCrop.c_str());
  display.printf("T:%.1fC H:%.1f%%\n", t, h);
  display.printf("Soil:%d%% LDR:%d\n", s, ldr_val);

  // Trạng thái
  display.printf("PUMP:%s LIG:%s ROOF:%d", isPumpOn ? "ON" : "OFF",
                 isLightOn ? "ON" : "OFF", roofAngle);
  display.display();
}

void setup_wifi() {
  Serial.print("\n" CLR_CYN "[SYSTEM] Connecting WiFi..." CLR_RST);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(CLR_GRN " OK!" CLR_RST);
  espClient.setInsecure();

  // Cài đặt thư viện lấy thời gian Internet
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("\n" CLR_RED "[EROR] Failed to obtain time" CLR_RST);
  } else {
    Serial.println("\n" CLR_GRN "[SYSTEM] Time synced successfully!" CLR_RST);
  }
}

// Cập nhật trạng thái đầu ra vật lý
void updateHardware() {
  digitalWrite(PUMP_LED, isPumpOn ? HIGH : LOW);
  digitalWrite(LIGHT_LED, isLightOn ? HIGH : LOW);

  // Ghi trực tiếp góc vào Servo (đã sửa lỗi chặn logic)
  roofServo.write(roofAngle);
}

void callback(char *topic, byte *payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error)
    return;

  if (doc["cmd"] == "update_config") {
    if (doc["crop"].is<String>())
      currentCrop = doc["crop"].as<String>();
    if (doc["min"].is<int>())
      minMoisture = doc["min"];
    if (doc["max"].is<int>())
      maxMoisture = doc["max"];
    if (doc["mode"].is<String>())
      currentMode = doc["mode"].as<String>();
    Serial.println("\n" CLR_YLW
                   "[CONFIG] Data Updated from Dashboard!" CLR_RST);
  } else if (doc["cmd"] == "control" && currentMode == "MANUAL") {
    if (doc["pump"].is<String>())
      isPumpOn = (doc["pump"] == "ON");
    if (doc["light"].is<String>())
      isLightOn = (doc["light"] == "ON");
    if (doc["roof"].is<String>())
      roofAngle = (doc["roof"] == "OPEN") ? 90 : 0;

    updateHardware();
    Serial.println("\n" CLR_YLW "[MANUAL] Command Executed!" CLR_RST);
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Farm-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      client.subscribe("farm/config");
      client.subscribe("farm/manual");
      Serial.println("\n" CLR_GRN "[MQTT] Connected and Ready!" CLR_RST);
    } else {
      Serial.print(CLR_RED "." CLR_RST);
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PUMP_LED, OUTPUT);
  pinMode(LIGHT_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);


  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  roofServo.setPeriodHertz(50);
  roofServo.attach(SERVO_PIN, 500, 2400);
  roofAngle = 0;
  roofServo.write(roofAngle);

  dht.begin();
  setup_display();
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected())
    reconnect();
  client.loop();

  // Đọc dữ liệu
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t))
    return;

  int soil = map(readAverage(SOIL_PIN), 0, 4095, 0, 100);
  int ldr_value = readAverage(LDR_PIN);

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int hour = timeinfo.tm_hour;

  // Xử lý Logic AUTO
  if (currentMode == "AUTO") {
    // --- LỚP 1: BƠM TƯỚI NƯỚC ---
    bool inWateringTime = ((hour >= 5 && hour < 8) || (hour >= 16 && hour < 18));
    if (inWateringTime && minMoisture > 0) {
      if (soil < minMoisture)
        isPumpOn = true;
      else if (soil >= maxMoisture)
        isPumpOn = false;
    } else {
      isPumpOn = false;
    }

    // --- LỚP 2: QUẠT/MÁI CHE (SERVO) ---
  
    if (h >= 90.0) {
      roofAngle = 0; // Cực ẩm / Mưa to -> Đóng mái
    } else if (t >= 32.0) {
      roofAngle = 90; // Quá nóng -> Mở mái tản nhiệt
    } else if (h >= 75.0 && h < 90.0) {
      roofAngle = 90; // Hơi ẩm hầm bí -> Mở thông gió
    } else if (t <= 29.0 && h < 75.0) {
      roofAngle = 0; // Mát mẻ, khô ráo -> Đóng mái giữ nhiệt
    }

    // --- LỚP 3: CHIẾU SÁNG ---
    bool inNightRestTime = (hour >= 20 || hour < 5);
    if (inNightRestTime) {
      isLightOn = false;
    } else {
      if (ldr_value > 2000)
        isLightOn = true;
      else if (ldr_value < 1000)
        isLightOn = false;
    }

    // --- LỚP 4: CẢNH BÁO (CÒI) ---
    if (t > 35.0) {
      tone(BUZZER, 1500); // Kêu gắt báo nóng
    } else if (soil < 10) {
      tone(BUZZER, 800); // Kêu trầm báo khô
    } else {
      noTone(BUZZER);
    }

    updateHardware();
  } else if (currentMode == "MAINT." || currentMode == "WAIT") {
    // Tắt hết thiết bị khi bảo trì hoặc chờ lệnh
    isPumpOn = false;
    isLightOn = false;
    roofAngle = 0;
    noTone(BUZZER);
    updateHardware();
  }

  update_oled(t, h, soil, ldr_value, timeinfo);

  // TRÌNH BÀY TERMINAL 
  String currentAlert = (t > 35.0) ? "HOT" : (soil < 10 ? "DRY" : "OK");

  bool sensorsChanged =
      (abs(t - lastSentT) > 0.5 || abs(h - lastSentH) > 1.0 ||
       abs(soil - lastSentSoil) > 1 || abs(ldr_value - lastSentLdr) > 50);

  bool devicesChanged =
      (isPumpOn != lastSentPump || isLightOn != lastSentLight ||
       roofAngle != lastSentRoof || currentAlert != lastSentAlert);

  if (sensorsChanged || devicesChanged || (millis() - lastSend > 30000)) {

    // 1. In Serial (ANSI Colors + Clean Block Format)
    Serial.println(
        "\n" CLR_CYN
        "==================== [ STATE UPDATE ] ====================" CLR_RST);
    Serial.printf("Time: %02d:%02d:%02d | Mode: %s | Crop: %s\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                  currentMode.c_str(), currentCrop.c_str());

    Serial.print("Sensors: ");
    if (t > 35.0)
      Serial.print(CLR_RED);
    else
      Serial.print(CLR_GRN);
    Serial.printf("Temp: %.1fC " CLR_RST, t);
    Serial.printf("| Humi: %.1f%% | ", h);

    if (soil < 10)
      Serial.print(CLR_RED);
    else
      Serial.print(CLR_BLU);
    Serial.printf("Soil: %d%% " CLR_RST, soil);
    Serial.printf("| LDR: %d\n", ldr_value);

    Serial.printf("Devices: Pump [%s] | Light [%s] | Roof Servo [%d deg]\n",
                  isPumpOn ? "ON" : "OFF", isLightOn ? "ON" : "OFF", roofAngle);

    if (currentAlert != "OK") {
      Serial.print(CLR_YLW "CRITICAL: " CLR_RST);
      Serial.println((currentAlert == "HOT") ? "High Temperature Alert!"
                                             : "Soil Moisture Low Alert!");
    }
    Serial.println(
        CLR_CYN
        "==========================================================" CLR_RST);

    // 2. Gửi MQTT
    JsonDocument docOut;
    docOut["temp"] = t;
    docOut["humi"] = h;
    docOut["soil"] = soil;
    docOut["ldr"] = ldr_value;
    docOut["pump"] = isPumpOn ? "ON" : "OFF";
    docOut["light"] = isLightOn ? "ON" : "OFF";
    docOut["roof"] = (roofAngle > 0) ? "OPEN" : "CLOSED";
    docOut["alert"] = (t > 35.0) ? "Nhiệt độ quá cao!"
                                 : (soil < 10 ? "Đất quá khô!" : "Bình thường");

    char buffer[256];
    serializeJson(docOut, buffer);
    client.publish("farm/sensors", buffer);

    // Lưu lại trạng thái vừa gửi
    lastSentT = t;
    lastSentH = h;
    lastSentSoil = soil;
    lastSentLdr = ldr_value;
    lastSentPump = isPumpOn;
    lastSentLight = isLightOn;
    lastSentRoof = roofAngle;
    lastSentAlert = currentAlert;
    lastSend = millis();


    updateHardware();
  }
}