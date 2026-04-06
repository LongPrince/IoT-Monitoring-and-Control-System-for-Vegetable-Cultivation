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

3. Kịch bản hoạt động (Logic Control)
Bạn có thể lập trình ESP32 chạy theo các quy tắc tự động (Auto Mode) như sau:

Tưới nước tự động: Nếu giá trị đọc từ Biến trở (độ ẩm đất) < 40% ➡️ Bật LED Xanh (Bơm nước). Khi độ ẩm > 70% ➡️ Tắt LED Xanh.

Điều hòa nhiệt độ: Nếu DHT22 báo nhiệt độ > 32°C ➡️ Bật LED Trắng/Servo (Quạt). Nếu < 28°C ➡️ Tắt Quạt.

Bù sáng: Nếu LDR báo trời tối (giá trị ánh sáng thấp) ➡️ Bật LED Vàng (Đèn quang hợp).

Cảnh báo an toàn: Nếu nhiệt độ vượt 40°C (nguy hiểm cho cây) ➡️ Hú còi Buzzer và gửi cảnh báo khẩn cấp lên MQTT.
