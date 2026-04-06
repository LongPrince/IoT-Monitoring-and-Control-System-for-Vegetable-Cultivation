Thiết kế và triển khai hệ thống IoT giám sát và điều khiển môi trường trồng rau sạch

I. Tổng quan
- Giám sát 4 nhiệm vụ chính
	+ Nhiệt độ
	+ Độ ẩm không khí
	+ Độ ẩm đất
	+ Ánh sáng
Mô tả: Đo nhiệt độ,độ ẩm,ánh sáng.Dựa vào số liệu đó tiến hành ON/OFF thiết bị điều khiển sao cho phù hợp với tình trạng thực tế.
- Thiết bị
- Nhóm Cảm biến (Đầu vào):
	+ Nhiệt độ,độ ẩm không khí: DHT22
	+ Ánh sáng: Photoresistor / LDR
	+ Độ ẩm đất: Soil Moisture Sensor .Giả lập bằng biến trở Potentiometer để thay đổi giá trị

- Nhóm Chấp hành (Đầu ra):
	+ Máy bơm nước (Water Pump):Giả lập bằng LED Xanh dương (Blue LED). Sáng lên nghĩa là đang tưới cây.
	+ Quạt thông gió (Fan): Giả lập bằng Động cơ Servo hoặc LED Trắng. Hoạt động khi nhiệt độ quá cao.
	+ Đèn quang hợp (Grow Light): Giả lập bằng LED Vàng (Yellow LED). Sáng lên khi thiếu ánh sáng mặt trời.
	+ Còi báo động (Buzzer): Có thể giữ lại từ dự án cũ để hú còi cảnh báo khi nhiệt độ quá cao hoặc độ ẩm quá thấp mà hệ thống bơm/quạt bị lỗi.

- Nhóm Giao tiếp & Hiển thị:
	+ Màn hình LCD 16x2 I2C: Hiển thị trực tiếp các thông số ngay tại vườn (rất trực quan trên Wokwi).

	+ ESP32 & MQTT (HiveMQ): Giữ nguyên cấu trúc kết nối WiFi và MQTT như dự án trước của bạn.
II.Loigic Của Hệ Thống 

1. Các Chế độ
 Với 3 chế độ cơ bản như sau:
 + Tự động : - Chọn loại rau thích hợp để
			 - Xác nhận và tiến hành hệ thống
 + Thủ công: ON/OF từng thiết bị
 + Bảo trì: OFF toàn bộ hệ thống

2. Hệ thống Tưới nước (Máy bơm - LED Xanh)
Chịu trách nhiệm giữ cho rễ cây luôn đủ nước, không bị khô héo cũng không bị ngập úng.
- Cảm biến tham chiếu: Cảm biến độ ẩm đất (Giả lập bằng Biến trở).
- Kịch bản:
   + Khi đất khô (Dưới 40%): BẬT máy bơm.
   + Khi đất ướt (Trên 70%): TẮT máy bơm.
Trong khoảng 40% - 70%: Bơm giữ nguyên trạng thái hiện tại (Nếu đang bơm thì vẫn tiếp tục bơm cho đến khi đạt 70%, nếu đang tắt thì vẫn tắt cho đến khi tụt xuống 40%).
- Tưới cây theo thời gian sáng sớm và chiều muộn kết hợp với kiểm tra độ ẩm
- Tưới cây theo độ ẩm thích hợp cho từng loại rau

3. Hệ thống Thông gió / Làm mát (Mái che - Servo)
Chịu trách nhiệm điều hòa không khí trong nhà kính, giúp tản nhiệt khi trời nắng gắt.
- Cảm biến tham chiếu: Cảm biến DHT22 (Đọc nhiệt độ & Độ ẩm không khí).
- Kịch bản:
  + Trời nóng (Nhiệt độ > 32°C): MỞ mái che (Servo quay góc 90°).
  + Trời mát (Nhiệt độ < 29°C): ĐÓNG mái che (Servo quay góc 0°).

Bảo vệ khi trời mưa (Độ ẩm không khí > 90%): Cưỡng chế ĐÓNG mái che bất kể nhiệt độ đang là bao nhiêu để tránh mưa hắt vào vườn.

4. Hệ thống Chiếu sáng (Đèn quang hợp - LED Vàng)
Bù đắp ánh sáng cho cây khi trời sập tối hoặc nhiều mây.
Cảm biến tham chiếu: Quang trở LDR.
- Kịch bản:
Nếu trời buổi sáng:
 + Trời tối (Giá trị LDR > 2000): BẬT đèn.
 + Trời sáng (Giá trị LDR < 2000): TẮT đèn.
 + Nếu trời buổi tối lấy thời gian thực để tắt đèn

III.Giải pháp điều khiển từ xa

- Hiển thị biểu đồ hệ thống lên web: Dotnet. App điện thoại : MQtt and Blynk IoT (hiển thị biểu đồ)
- Gửi cảnh báo về mail
- Điều khiển thiết bị từ xa
- Kết nối CSDL Firebase

-- UPDATE 6/4
-Kết nối HvMQ và cơ sở dữ liệu Firebase triển khai trên Node-RED
+ Tiến hành cấu hình CSDL Firebase trên Realtime Database 
![alt text](image-1.png)
 + Data_Firebase_LoaiRau.json: Chứa độ ẩm thích hợp cho từng loại rau
 + 
+ Cài đặt thư viên cho Node-RED : 
  + node-red-contrib-firebase.
  + /SmartFarm.json:  URL database để cấu hình Firebase cho Node-RED

![alt text](image-2.png)
+ Chạy thử nghiệm và cho ra kết quả KẾT NỐI THÀNH CÔNG
![alt text](image.png)

![alt text](image-3.png)