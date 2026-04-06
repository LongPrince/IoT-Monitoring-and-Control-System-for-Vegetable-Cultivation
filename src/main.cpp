#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <DHT.h>

// --- KHAI BÁO CHÂN ---
#define DHT_PIN       15    
#define SOIL_PIN      34    
#define LDR_PIN       35    

#define PUMP_PIN      16    // Bơm nước (LED Xanh)
#define SERVO_PIN     17    // Cửa thông gió (Servo)
#define LIGHT_PIN     18    // Đèn quang hợp (LED Vàng)
#define BUZZER_PIN    19    // Còi

// --- CẤU HÌNH CẢM BIẾN & MODULE ---
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo ventServo; 

// --- NGƯỠNG CÀI ĐẶT ---
const int SOIL_DRY = 40;   
const int SOIL_WET = 70;   
const float TEMP_HOT = 32.0; 
const float TEMP_SAFE = 29.0; 
const int LIGHT_DARK = 2000;  
const float TEMP_DANGER = 40.0;

// Trạng thái hệ thống
bool isPumpOn = false;
bool isVentOpen = false;
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  dht.begin();
  
  // Khởi tạo Servo ở góc 0 độ (Đóng mái)
  ventServo.attach(SERVO_PIN);
  ventServo.write(0); 
  
  // Khởi tạo OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Địa chỉ I2C của OLED là 0x3C
    Serial.println("Lỗi khởi tạo màn hình OLED");
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25);
  display.print("SMART FARM SYSTEM");
  display.display();
  delay(2000);
}

void loop() {
  if (millis() - lastReadTime > 2000) {
    lastReadTime = millis();
    
    // 1. ĐỌC DỮ LIỆU
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int rawSoil = analogRead(SOIL_PIN);
    int soilPercent = map(rawSoil, 0, 4095, 0, 100); 
    int rawLight = analogRead(LDR_PIN);

    if (isnan(t) || isnan(h)) return; // Bỏ qua nếu lỗi

    // 2. LOGIC ĐIỀU KHIỂN
    
    // Bơm nước
    if (soilPercent <= SOIL_DRY && !isPumpOn) {
      digitalWrite(PUMP_PIN, HIGH);
      isPumpOn = true;
    } else if (soilPercent >= SOIL_WET && isPumpOn) {
      digitalWrite(PUMP_PIN, LOW);
      isPumpOn = false;
    }

    // Mở mái/Cửa thông gió (Servo)
    if (t >= TEMP_HOT && !isVentOpen) {
      ventServo.write(90); // Mở cửa 90 độ
      isVentOpen = true;
      Serial.println("Trời nóng: MỞ CỬA THÔNG GIÓ");
    } else if (t <= TEMP_SAFE && isVentOpen) {
      ventServo.write(0); // Đóng cửa
      isVentOpen = false;
      Serial.println("Nhiệt độ ổn: ĐÓNG CỬA");
    }

    // Đèn
    digitalWrite(LIGHT_PIN, rawLight > LIGHT_DARK ? HIGH : LOW);

    // Còi
    if (t >= TEMP_DANGER) tone(BUZZER_PIN, 1000);
    else noTone(BUZZER_PIN);

    // 3. HIỂN THỊ LÊN OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    
    // In thông số
    display.printf("Temp: %.1f C\n", t);
    display.printf("Humi: %.1f %%\n", h);
    display.printf("Soil: %d %%\n\n", soilPercent);
    
    // In trạng thái thiết bị
    display.print("Pump: ");
    display.println(isPumpOn ? "ON" : "OFF");
    
    display.print("Vent: ");
    display.println(isVentOpen ? "OPEN (90')" : "CLOSED (0')");
    
    display.display();
  }
}