#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> // 1. Thêm thư viện này cho kết nối bảo mật (Port 8883)
#include <PubSubClient.h>
#include <ArduinoJson.h> // Thư viện xử lý JSON

// Cấu hình WiFi (Mặc định của Wokwi)
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Cấu hình MQTT (Đổi thành broker của bạn)
const char* mqtt_server = "*";
const char* mqtt_user   = "*";
const char* mqtt_pass   = "*";
const int mqtt_port = 8883;

// Topic giao tiếp
const char* topic_config = "farm/config";   // Nhận cấu hình từ Node-RED
const char* topic_status = "farm/status";   // Gửi báo cáo lên Node-RED

// 2. Đổi WiFiClient thành WiFiClientSecure
WiFiClientSecure espClient; 
PubSubClient client(espClient);

// --- CÁC BIẾN LƯU TRỮ CẤU HÌNH HIỆN TẠI ---
String currentCrop = "Chưa có dữ liệu";
int minMoisture = 0;
int maxMoisture = 0;
String currentMode = "WAIT"; // Đợi cấu hình

// Hàm kết nối WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối thành công!");

  // 3. Bỏ qua xác thực chứng chỉ SSL (Cần thiết khi dùng HiveMQ Cloud để tránh lỗi xác thực)
  espClient.setInsecure(); 
}

// Hàm Callback - KHI CÓ TIN NHẮN TỪ MQTT GỬI XUỐNG SẼ CHẠY VÀO ĐÂY
void callback(char* topic, byte* payload, unsigned int length) {
  // Chuyển byte payload thành chuỗi String
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  Serial.println("-----------------------");
  Serial.print("Nhận được tin nhắn từ Topic: ");
  Serial.println(topic);
  Serial.println("Nội dung: " + messageTemp);

  // Nếu tin nhắn thuộc topic cấu hình
  if (String(topic) == topic_config) {
    // Khai báo bộ nhớ đệm cho ArduinoJson (khoảng 256 byte là đủ cho file nhỏ)
    StaticJsonDocument<256> doc;
    
    // Phân tích chuỗi JSON
    DeserializationError error = deserializeJson(doc, messageTemp);
    
    if (error) {
      Serial.print("Lỗi đọc JSON: ");
      Serial.println(error.c_str());
      return;
    }

    // Bóc tách dữ liệu JSON và gán vào biến hệ thống
    const char* cmd = doc["cmd"];
    if (String(cmd) == "update_config") {
      currentCrop = doc["crop"].as<String>();
      minMoisture = doc["min"];
      maxMoisture = doc["max"];
      currentMode = doc["mode"].as<String>();

      Serial.println(">>> CẬP NHẬT CẤU HÌNH THÀNH CÔNG <<<");
      Serial.println("Loại rau: " + currentCrop);
      Serial.print("Ngưỡng tưới: ");
      Serial.print(minMoisture);
      Serial.print("% -> ");
      Serial.print(maxMoisture);
      Serial.println("%");
      Serial.println("Chế độ: " + currentMode);
    }
  }
}

// Hàm kết nối lại MQTT nếu bị rớt mạng
void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối MQTT...");
    String clientId = "ESP32FarmClient-" + String(random(0xffff), HEX);
    
    // 4. Thêm mqtt_user và mqtt_pass vào hàm connect để xác thực với HiveMQ
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Đã kết nối!");
      // Đăng ký lắng nghe Topic cấu hình
      client.subscribe(topic_config);
      
      // Gửi tin nhắn "hello" lên để kích hoạt Node-RED nhả cấu hình xuống
      Serial.println("Gửi tín hiệu báo thức cho Node-RED...");
      client.publish(topic_status, "hello");
    } else {
      Serial.print("Thất bại, mã lỗi=");
      Serial.print(client.state());
      Serial.println(" Thử lại sau 5 giây");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Đăng ký hàm nhận tin nhắn
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ở đây bạn sẽ viết logic điều khiển Bơm, Đèn, Servo
  // Sử dụng biến currentCrop, minMoisture, maxMoisture
  // Ví dụ:
  /*
  if (currentMode == "AUTO") {
     int soilHumi = getSoilMoisture();
     if (soilHumi < minMoisture) { turnOnPump(); }
     if (soilHumi >= maxMoisture) { turnOffPump(); }
  }
  */
}