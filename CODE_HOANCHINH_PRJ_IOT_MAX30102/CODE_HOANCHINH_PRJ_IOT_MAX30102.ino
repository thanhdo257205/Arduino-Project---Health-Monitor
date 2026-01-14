/*
 * ============================================
 *    HỆ THỐNG ĐO SỨC KHỎE IoT - NHÓM 5
 *    CÁC THÀNH VIÊN: ĐỖ HOÀN THÀNH
 *                    NGUYỄN ĐỖ ĐÌNH ANH
 * ============================================
 * 
 * CHỨC NĂNG:
 * 1. Cơ bản: 
 *    - Đọc nhiệt độ LM35
 *    - Đọc nhịp tim và SpO2 từ MAX30102
 *    - Hiển thị lên LCD 16x2
 *    - Cảnh báo bất thường bằng đèn LED và còi
 * 
 * 2. Nâng cao:
 *    - Gửi dữ liệu qua UART để lưu file
 *    - Tự động đo 20 giây khi đặt ngón tay
 *    - Tính trung bình sau khi đo xong
 *    - 3 đèn LED hiển thị tiến trình và kết quả
 * 
 * SƠ ĐỒ KẾT NỐI:
 *    LM35:      VCC->5V, OUT->A0, GND->GND
 *    MAX30102:  VIN->3.3V, GND->GND, SDA->A4, SCL->A5
 *    LCD I2C:   VCC->5V, GND->GND, SDA->A4, SCL->A5
 *    Buzzer:    (+)->D8, (-)->GND
 *    LED Xanh:  (+)->D9 qua 220Ω, (-)->GND
 *    LED Vàng:  (+)->D10 qua 220Ω, (-)->GND
 *    LED Đỏ:    (+)->D11 qua 220Ω, (-)->GND
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30105.h"

// ==================== CẤU HÌNH CHÂN ====================
#define LM35_PIN        A0      // Chân đọc cảm biến nhiệt độ
#define BUZZER_PIN      8       // Chân điều khiển còi
#define LED_GREEN_PIN   9       // Đèn xanh - Bình thường
#define LED_YELLOW_PIN  10      // Đèn vàng - Cảnh báo
#define LED_RED_PIN     11      // Đèn đỏ - Nguy hiểm

// ==================== NGƯỠNG CẢNH BÁO ====================
// Mức ĐỎ (Nguy hiểm)
#define HR_WARN_LOW       50      // Nhịp tim quá chậm
#define HR_WARN_HIGH      120     // Nhịp tim quá nhanh
#define SPO2_WARN         90      // SpO2 quá thấp
#define TEMP_WARN_LOW     35.0    // Nhiệt độ quá thấp
#define TEMP_WARN_HIGH    38.5    // Nhiệt độ quá cao (sốt cao)

// Mức XANH (Bình thường)
#define HR_MIN            60      // Nhịp tim tối thiểu bình thường
#define HR_MAX            100     // Nhịp tim tối đa bình thường
#define SPO2_MIN          95      // SpO2 tối thiểu bình thường
#define TEMP_MIN          36.0    // Nhiệt độ tối thiểu bình thường
#define TEMP_MAX          37.5    // Nhiệt độ tối đa bình thường

// ==================== CẤU HÌNH ĐO ====================
#define MEASURE_TIME      20000   // Thời gian đo:  20 giây
#define SAMPLE_RATE       1000    // Lấy mẫu mỗi 1 giây

// Chia 20 giây thành 3 giai đoạn cho đèn LED
#define PHASE_1_TIME      7000    // 0-7s: Đèn xanh
#define PHASE_2_TIME      14000   // 7-14s: Đèn xanh + vàng
#define PHASE_3_TIME      20000   // 14-20s: Đèn xanh + vàng + đỏ

// Thời gian hiển thị kết quả
#define RESULT_DISPLAY_TIME  10000  // 10 giây hiển thị kết quả

// ==================== KHỞI TẠO MODULE ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD địa chỉ 0x27, 16 cột, 2 hàng
MAX30105 sensor;                      // Cảm biến MAX30102

// ==================== BIẾN TÍNH NHỊP TIM ====================
long irPrev = 0;              // Giá trị IR trước đó
long irMin = 999999;          // Giá trị IR nhỏ nhất
long irMax = 0;               // Giá trị IR lớn nhất
long irThreshold = 0;         // Ngưỡng phát hiện nhịp tim

unsigned long lastBeatTime = 0;  // Thời điểm nhịp tim cuối
int bpm = 0;                     // Nhịp tim tức thời

#define BPM_SIZE 5               // Số mẫu BPM để tính trung bình
int bpmArray[BPM_SIZE];          // Mảng lưu BPM
int bpmIndex = 0;                // Vị trí hiện tại trong mảng
int avgBPM = 0;                  // Nhịp tim trung bình

// ==================== BIẾN TÍNH SPO2 ====================
long redSum = 0;              // Tổng giá trị LED đỏ
long irSum = 0;               // Tổng giá trị hồng ngoại
int sampleCount = 0;          // Số mẫu đã lấy
int spo2 = 0;                 // Giá trị SpO2 hiện tại

// ==================== BIẾN LƯU TRỮ KẾT QUẢ ====================
#define MAX_SAMPLES 25           // Tối đa 25 mẫu (đủ cho 20 giây)
float tempData[MAX_SAMPLES];     // Mảng lưu nhiệt độ
int hrData[MAX_SAMPLES];         // Mảng lưu nhịp tim
int spo2Data[MAX_SAMPLES];       // Mảng lưu SpO2
int dataIndex = 0;               // Vị trí hiện tại trong mảng

// ==================== BIẾN TRẠNG THÁI ====================
bool measuring = false;              // Đang đo hay không
bool showingResult = false;          // Đang hiển thị kết quả
unsigned long resultStartTime = 0;   // Thời điểm bắt đầu hiển thị kết quả
unsigned long startTime = 0;         // Thời điểm bắt đầu đo
unsigned long lastSample = 0;        // Thời điểm lấy mẫu cuối
int currentPhase = 0;                // Giai đoạn hiện tại (1, 2, 3)
int healthStatus = 0;                // Trạng thái sức khỏe (0=OK, 1=WARN, 2=DANGER)

// ==================== KÝ TỰ ĐẶC BIỆT LCD ====================
// Ký tự trái tim
byte heartChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

// Ký tự độ (°)
byte degreeChar[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// Ký tự thanh tiến trình
byte progressChar[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

// ==================== HÀM SETUP ====================
void setup() {
  // Khởi tạo Serial
  Serial.begin(9600);
  
  // Cấu hình các chân
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LM35_PIN, INPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  
  // Tắt tất cả đèn và buzzer
  digitalWrite(BUZZER_PIN, LOW);
  turnOffAllLeds();
  
  // Khởi tạo I2C
  Wire.begin();
  
  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, heartChar);
  lcd.createChar(1, degreeChar);
  lcd.createChar(2, progressChar);
  
  // Hiển thị màn hình khởi động
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Health Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Khoi dong...");
  
  // Test đèn LED tuần tự
  testLedsSequence();
  
  // Khởi tạo cảm biến MAX30102
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    // không tìm thấy sensor
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LOI SENSOR!");
    lcd.setCursor(0, 1);
    lcd.print("Kiem tra day noi");
    
    Serial.println(F("LOI:  Khong tim thay MAX30102! "));
    Serial.println(F("Kiem tra ket noi: "));
    Serial.println(F("  VIN -> 3.3V"));
    Serial.println(F("  GND -> GND"));
    Serial.println(F("  SDA -> A4"));
    Serial.println(F("  SCL -> A5"));
    
    // Nhấp nháy đèn báo lỗi
    while(1) {
      blinkAllLeds();
      tone(BUZZER_PIN, 500, 300);
      delay(500);
    }
  }
  
  // Cấu hình sensor
  sensor.setup();
  sensor.setPulseAmplitudeRed(0x1F);
  sensor.setPulseAmplitudeIR(0x1F);
  
  // Reset tất cả dữ liệu
  resetAllData();
  
  // In thông tin qua UART
  Serial.println(F(""));
  Serial.println(F("================================================"));
  Serial.println(F("       HE THONG DO SUC KHOE IoT"));
  Serial.println(F("================================================"));
  Serial.println(F(""));
  Serial.println(F("Tien trinh do 20 giay: "));
  Serial.println(F("  0-7s  : DEN XANH sang"));
  Serial.println(F("  7-14s : DEN XANH + VANG sang"));
  Serial.println(F("  14-20s:  DEN XANH + VANG + DO sang"));
  Serial.println(F(""));
  Serial.println(F("Ket qua: "));
  Serial.println(F("  XANH :  Binh thuong"));
  Serial.println(F("  VANG :  Canh bao (nhap nhay)"));
  Serial.println(F("  DO   : Nguy hiem (nhap nhay)"));
  Serial.println(F(""));
  Serial.println(F("Format CSV:  Time(ms),Temp(C),HR(BPM),SpO2(%),Phase"));
  Serial.println(F("------------------------------------------------"));
  
  delay(1000);
  
  // Hiển thị màn hình sẵn sàng
  showReady();
  beepReady();
}

// ==================== TEST ĐÈN LED TUẦN TỰ ====================
void testLedsSequence() {
  // Bật lần lượt từng đèn
  digitalWrite(LED_GREEN_PIN, HIGH);
  tone(BUZZER_PIN, 1000, 100);
  delay(400);
  
  digitalWrite(LED_YELLOW_PIN, HIGH);
  tone(BUZZER_PIN, 1200, 100);
  delay(400);
  
  digitalWrite(LED_RED_PIN, HIGH);
  tone(BUZZER_PIN, 1500, 100);
  delay(400);
  
  // Tắt tất cả
  turnOffAllLeds();
  delay(200);
}

// ==================== NHẤP NHÁY TẤT CẢ ĐÈN ====================
void blinkAllLeds() {
  static bool state = false;
  state = !state;
  digitalWrite(LED_GREEN_PIN, state);
  digitalWrite(LED_YELLOW_PIN, state);
  digitalWrite(LED_RED_PIN, state);
}

// ==================== TẮT TẤT CẢ ĐÈN ====================
void turnOffAllLeds() {
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
}

// ==================== CẬP NHẬT ĐÈN THEO TIẾN TRÌNH ĐO ====================
void updateProgressLeds(unsigned long elapsed) {
  int newPhase = 0;
  
  if (elapsed < PHASE_1_TIME) {
    // Giai đoạn 1: 0-7s → Chỉ đèn XANH
    newPhase = 1;
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    
  } else if (elapsed < PHASE_2_TIME) {
    // Giai đoạn 2: 7-14s → XANH + VÀNG
    newPhase = 2;
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_YELLOW_PIN, HIGH);
    digitalWrite(LED_RED_PIN, LOW);
    
  } else {
    // Giai đoạn 3: 14-20s → XANH + VÀNG + ĐỎ
    newPhase = 3;
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_YELLOW_PIN, HIGH);
    digitalWrite(LED_RED_PIN, HIGH);
  }
  
  // Beep khi chuyển giai đoạn
  if (newPhase != currentPhase) {
    currentPhase = newPhase;
    tone(BUZZER_PIN, 1000 + (currentPhase * 200), 100);
    
    Serial.print(F(">> Giai doan "));
    Serial.print(currentPhase);
    Serial.println(F("/3"));
  }
}

// ==================== NHẤP NHÁY ĐÈN ĐỎ ====================
void blinkRedLed() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > 200) {
    ledState = !ledState;
    digitalWrite(LED_RED_PIN, ledState);
    
    // Thêm beep khi đèn sáng
    if (ledState) {
      tone(BUZZER_PIN, 1000, 50);
    }
    
    lastBlink = millis();
  }
}

// ==================== NHẤP NHÁY ĐÈN VÀNG ====================
void blinkYellowLed() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > 400) {
    ledState = !ledState;
    digitalWrite(LED_YELLOW_PIN, ledState);
    lastBlink = millis();
  }
}

// ==================== TÍNH MỨC CẢNH BÁO SỨC KHỎE ====================
int calculateHealthStatus(float temp, int hr, int sp) {
  int status = 0;  // Mặc định:  OK (Xanh)
  
  // === KIỂM TRA MỨC ĐỎ (Nguy hiểm) ===
  if (temp > 0 && (temp < TEMP_WARN_LOW || temp > TEMP_WARN_HIGH)) {
    status = 2;
  }
  if (hr > 0 && (hr < HR_WARN_LOW || hr > HR_WARN_HIGH)) {
    status = 2;
  }
  if (sp > 0 && sp < SPO2_WARN) {
    status = 2;
  }
  
  // Nếu đã là mức đỏ, return luôn
  if (status == 2) return status;
  
  // === KIỂM TRA MỨC VÀNG (Cảnh báo) ===
  if (temp > 0 && (temp < TEMP_MIN || temp > TEMP_MAX)) {
    status = 1;
  }
  if (hr > 0 && (hr < HR_MIN || hr > HR_MAX)) {
    status = 1;
  }
  if (sp > 0 && sp < SPO2_MIN) {
    status = 1;
  }
  
  return status;
}

// ==================== VÒNG LẶP CHÍNH ====================
void loop() {
  // ========== XỬ LÝ HIỂN THỊ KẾT QUẢ ==========
  if (showingResult) {
    // Nhấp nháy đèn theo trạng thái sức khỏe
    if (healthStatus == 2) {
      blinkRedLed();      // Đỏ nhấp nháy + beep
    } else if (healthStatus == 1) {
      blinkYellowLed();   // Vàng nhấp nháy
    }
    // Xanh sáng đều (đã bật trong finishMeasurement)
    
    // Kiểm tra hết thời gian hiển thị
    if (millis() - resultStartTime > RESULT_DISPLAY_TIME) {
      showingResult = false;
      healthStatus = 0;
      showReady();
    }
    
    delay(10);
    return;  // Không làm gì khác khi đang hiển thị kết quả
  }
  
  // ========== ĐỌC GIÁ TRỊ SENSOR ==========
  long irValue = sensor.getIR();
  bool fingerDetected = (irValue > 50000);
  
  // ========== BẮT ĐẦU ĐO KHI ĐẶT NGÓN TAY ==========
  if (fingerDetected && ! measuring) {
    startMeasurement();
  }
  
  // ========== HỦY ĐO KHI BỎ NGÓN TAY ==========
  if (!fingerDetected && measuring) {
    static unsigned long lostTime = 0;
    if (lostTime == 0) {
      lostTime = millis();
    } else if (millis() - lostTime > 2000) {
      // Mất tín hiệu > 2 giây → Hủy đo
      lostTime = 0;
      cancelMeasurement();
      return;
    }
  } else {
    static unsigned long lostTime = 0;
    lostTime = 0;
  }
  
  // ========== XỬ LÝ KHI ĐANG ĐO ==========
  if (measuring) {
    unsigned long elapsed = millis() - startTime;
    
    if (elapsed < MEASURE_TIME) {
      // Cập nhật sensor liên tục
      long redValue = sensor.getRed();
      calculateHeartRate(irValue);
      calculateSpO2(irValue, redValue);
      
      // Lấy mẫu mỗi 1 giây
      if (millis() - lastSample >= SAMPLE_RATE) {
        collectSample();
        lastSample = millis();
      }
      
      // Cập nhật đèn LED theo tiến trình
      updateProgressLeds(elapsed);
      
      // Cập nhật LCD
      updateDisplay(elapsed);
      
    } else {
      // Hết 20 giây → Kết thúc đo
      finishMeasurement();
    }
  }
  
  delay(10);
}

// ==================== BẮT ĐẦU ĐO ====================
void startMeasurement() {
  measuring = true;
  showingResult = false;
  startTime = millis();
  lastSample = millis();
  dataIndex = 0;
  currentPhase = 0;
  
  // Reset dữ liệu
  resetAllData();
  
  // Tắt tất cả đèn
  turnOffAllLeds();
  
  // Hiển thị LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bat dau do.. .");
  lcd.setCursor(0, 1);
  lcd.print("Giu yen ngon tay");
  
  // Gửi UART
  Serial.println(F(""));
  Serial.println(F(">>> BAT DAU DO <<<"));
  Serial.println(F("Time,Temp,HR,SpO2,Phase"));
  
  // Âm thanh bắt đầu
  beepStart();
  delay(500);
}

// ==================== THU THẬP MẪU ====================
void collectSample() {
  if (dataIndex >= MAX_SAMPLES) return;
  
  // Đọc nhiệt độ
  float temp = readTemperature();
  
  // Lưu dữ liệu vào mảng
  tempData[dataIndex] = temp;
  hrData[dataIndex] = avgBPM;
  spo2Data[dataIndex] = spo2;
  dataIndex++;
  
  // Gửi dữ liệu qua UART (format CSV)
  unsigned long elapsed = millis() - startTime;
  Serial.print(elapsed);
  Serial.print(",");
  Serial.print(temp, 1);
  Serial.print(",");
  Serial.print(avgBPM);
  Serial.print(",");
  Serial.print(spo2);
  Serial.print(",");
  Serial.println(currentPhase);
}

// ==================== ĐỌC NHIỆT ĐỘ TỪ LM35 ====================
float readTemperature() {
  // Đọc nhiều lần và lấy trung bình để giảm nhiễu
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LM35_PIN);
    delay(1);
  }
  float avgReading = sum / 10.0;
  
  // Chuyển đổi:  LM35 cho 10mV/°C
  // Arduino 5V: temp = (reading * 500) / 1024
  float tempC = (avgReading * 500.0) / 1024.0;
  
  return tempC;
}

// ==================== TÍNH NHỊP TIM ====================
void calculateHeartRate(long irValue) {
  // Cập nhật giá trị min/max
  if (irValue < irMin) irMin = irValue;
  if (irValue > irMax) irMax = irValue;
  
  // Tính ngưỡng = trung bình của min và max
  irThreshold = (irMin + irMax) / 2;
  
  // Phát hiện đỉnh (peak) - khi IR vượt qua ngưỡng từ dưới lên
  if (irValue > irThreshold && irPrev <= irThreshold) {
    unsigned long now = millis();
    
    if (lastBeatTime > 0) {
      unsigned long interval = now - lastBeatTime;
      
      // Khoảng cách hợp lệ:  300ms - 1500ms (40 - 200 BPM)
      if (interval > 300 && interval < 1500) {
        bpm = 60000 / interval;
        
        // Lọc giá trị bất thường
        if (bpm >= 40 && bpm <= 200) {
          // Lưu vào mảng vòng tròn
          bpmArray[bpmIndex] = bpm;
          bpmIndex = (bpmIndex + 1) % BPM_SIZE;
          
          // Tính trung bình
          int sum = 0;
          int count = 0;
          for (int i = 0; i < BPM_SIZE; i++) {
            if (bpmArray[i] > 0) {
              sum += bpmArray[i];
              count++;
            }
          }
          if (count > 0) {
            avgBPM = sum / count;
          }
        }
      }
    }
    lastBeatTime = now;
  }
  
  irPrev = irValue;
  
  // Reset min/max mỗi 3 giây để thích ứng với thay đổi
  static unsigned long lastReset = 0;
  if (millis() - lastReset > 3000) {
    irMin = irValue - 1000;
    irMax = irValue + 1000;
    lastReset = millis();
  }
}

// ==================== TÍNH SPO2 ====================
void calculateSpO2(long irValue, long redValue) {
  // Tích lũy giá trị
  redSum += redValue;
  irSum += irValue;
  sampleCount++;
  
  // Tính SpO2 mỗi 50 mẫu
  if (sampleCount >= 50) {
    float avgRed = (float)redSum / sampleCount;
    float avgIR = (float)irSum / sampleCount;
    
    if (avgIR > 0 && avgRed > 0) {
      float ratio = avgRed / avgIR;
      
      // Công thức tính SpO2
      spo2 = 110 - 25 * ratio;
      
      // Giới hạn trong khoảng hợp lệ
      if (spo2 > 100) spo2 = 100;
      if (spo2 < 70) spo2 = 70;
    }
    
    // Reset
    redSum = 0;
    irSum = 0;
    sampleCount = 0;
  }
}

// ==================== CẬP NHẬT HIỂN THỊ LCD ====================
void updateDisplay(unsigned long elapsed) {
  int timeLeft = (MEASURE_TIME - elapsed) / 1000;
  float currentTemp = (dataIndex > 0) ? tempData[dataIndex - 1] : readTemperature();
  
  // Dòng 1: Nhiệt độ và nhịp tim
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(currentTemp, 1);
  lcd.write(1);  // Ký tự độ
  lcd.print(" ");
  lcd.write(byte(0));  // Ký tự trái tim
  lcd.print(":");
  if (avgBPM < 100) lcd.print(" ");
  if (avgBPM < 10) lcd.print(" ");
  lcd.print(avgBPM);
  lcd.print(" ");
  
  // Dòng 2: SpO2 và thanh tiến trình
  lcd.setCursor(0, 1);
  lcd.print("O2:");
  if (spo2 < 100) lcd.print(" ");
  lcd.print(spo2);
  lcd.print("% ");
  
  // Thanh tiến trình
  int progress = map(elapsed, 0, MEASURE_TIME, 0, 6);
  lcd.print("[");
  for (int i = 0; i < 6; i++) {
    if (i < progress) {
      lcd.write(2);  // Ký tự đầy
    } else {
      lcd.print("-");
    }
  }
  lcd.print("]");
}

// ==================== KẾT THÚC ĐO ====================
void finishMeasurement() {
  measuring = false;
  
  // Tính trung bình các giá trị
  float avgTemp = calcAvgFloat(tempData, dataIndex);
  int avgHR = calcAvgInt(hrData, dataIndex);
  int avgSpO2 = calcAvgInt(spo2Data, dataIndex);
  
  // Tính trạng thái sức khỏe
  healthStatus = calculateHealthStatus(avgTemp, avgHR, avgSpO2);
  
  // Bật chế độ hiển thị kết quả
  showingResult = true;
  resultStartTime = millis();
  
  // Tắt tất cả đèn trước
  turnOffAllLeds();
  
  // Bật đèn theo trạng thái
  if (healthStatus == 0) {
    digitalWrite(LED_GREEN_PIN, HIGH);  // Xanh sáng đều
  }
  // Vàng và đỏ sẽ nhấp nháy trong loop()
  
  // Hiển thị kết quả trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(avgTemp, 1);
  lcd.write(1);
  lcd.print(" ");
  lcd.write(byte(0));
  lcd.print(":");
  lcd.print(avgHR);
  
  lcd.setCursor(0, 1);
  lcd.print("O2:");
  lcd.print(avgSpO2);
  lcd.print("% ");
  
  // Hiển thị trạng thái
  switch(healthStatus) {
    case 0: lcd.print("[OK]"); break;
    case 1: lcd.print("[WARN]"); break;
    case 2: lcd.print("[!!! ]"); break;
  }
  
  // Gửi kết quả qua UART
  Serial.println(F(""));
  Serial.println(F("============ KET QUA TRUNG BINH ============"));
  Serial.print(F("Nhiet do: "));
  Serial.print(avgTemp, 1);
  Serial.println(F(" *C"));
  Serial.print(F("Nhip tim: "));
  Serial.print(avgHR);
  Serial.println(F(" BPM"));
  Serial.print(F("SpO2    : "));
  Serial.print(avgSpO2);
  Serial.println(F(" %"));
  Serial.print(F("So mau  : "));
  Serial.println(dataIndex);
  Serial.println(F("============================================="));
  
  // Phân tích và cảnh báo
  analyzeResults(avgTemp, avgHR, avgSpO2);
  
  // Âm thanh theo kết quả
  switch(healthStatus) {
    case 0: beepSuccess(); break;
    case 1: beepWarning(); break;
    case 2: beepDanger(); break;
  }
}

// ==================== HỦY ĐO ====================
void cancelMeasurement() {
  measuring = false;
  showingResult = false;
  currentPhase = 0;
  
  // Tắt tất cả đèn
  turnOffAllLeds();
  
  // Hiển thị thông báo
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DA HUY DO!");
  lcd.setCursor(0, 1);
  lcd.print("Giu yen ngon tay");
  
  Serial.println(F(""));
  Serial.println(F("! !! DA HUY - NGON TAY BO RA !!! "));
  
  beepError();
  delay(2000);
  showReady();
}

// ==================== PHÂN TÍCH KẾT QUẢ ====================
void analyzeResults(float temp, int hr, int sp) {
  Serial.println(F(""));
  Serial.println(F("------------- PHAN TICH -------------"));
  
  // Phân tích nhiệt độ
  Serial.print(F("Nhiet do: "));
  if (temp < TEMP_WARN_LOW) {
    Serial.println(F("NGUY HIEM - Ha than nhiet! "));
  } else if (temp < TEMP_MIN) {
    Serial.println(F("CANH BAO - Nhiet do thap"));
  } else if (temp > TEMP_WARN_HIGH) {
    Serial.println(F("NGUY HIEM - Sot cao! "));
  } else if (temp > TEMP_MAX) {
    Serial.println(F("CANH BAO - Sot nhe"));
  } else {
    Serial.println(F("BINH THUONG"));
  }
  
  // Phân tích nhịp tim
  Serial.print(F("Nhip tim: "));
  if (hr < HR_WARN_LOW) {
    Serial.println(F("NGUY HIEM - Nhip tim qua cham!"));
  } else if (hr < HR_MIN) {
    Serial.println(F("CANH BAO - Nhip tim cham"));
  } else if (hr > HR_WARN_HIGH) {
    Serial.println(F("NGUY HIEM - Nhip tim qua nhanh!"));
  } else if (hr > HR_MAX) {
    Serial.println(F("CANH BAO - Nhip tim nhanh"));
  } else {
    Serial.println(F("BINH THUONG"));
  }
  
  // Phân tích SpO2
  Serial.print(F("SpO2    : "));
  if (sp < SPO2_WARN) {
    Serial.println(F("NGUY HIEM - Thieu oxy nghiem trong!"));
  } else if (sp < SPO2_MIN) {
    Serial.println(F("CANH BAO - SpO2 thap"));
  } else {
    Serial.println(F("BINH THUONG"));
  }
  
  Serial.println(F("-------------------------------------"));
  
  // Kết luận
  Serial.print(F("KET QUA:  "));
  switch(healthStatus) {
    case 0:
      Serial.println(F("BINH THUONG [DEN XANH]"));
      break;
    case 1:
      Serial.println(F("CAN THEO DOI [DEN VANG NHAP NHAY]"));
      break;
    case 2:
      Serial.println(F("NGUY HIEM!  [DEN DO NHAP NHAY]"));
      break;
  }
  Serial.println(F(""));
}

// ==================== HIỂN THỊ MÀN HÌNH SẴN SÀNG ====================
void showReady() {
  // Tắt tất cả đèn, bật đèn xanh
  turnOffAllLeds();
  digitalWrite(LED_GREEN_PIN, HIGH);
  
  // Hiển thị LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  SAN SANG DO");
  lcd.setCursor(0, 1);
  lcd.print("Dat ngon tay.. .");
  
  Serial.println(F("San sang.  Dat ngon tay de do... "));
}

// ==================== HÀM TÍNH TRUNG BÌNH (FLOAT) ====================
float calcAvgFloat(float arr[], int count) {
  if (count == 0) return 0;
  
  float sum = 0;
  int valid = 0;
  
  for (int i = 0; i < count; i++) {
    if (arr[i] > 0) {
      sum += arr[i];
      valid++;
    }
  }
  
  return (valid > 0) ? (sum / valid) : 0;
}

// ==================== HÀM TÍNH TRUNG BÌNH (INT) ====================
int calcAvgInt(int arr[], int count) {
  if (count == 0) return 0;
  
  long sum = 0;
  int valid = 0;
  
  for (int i = 0; i < count; i++) {
    if (arr[i] > 0) {
      sum += arr[i];
      valid++;
    }
  }
  
  return (valid > 0) ? (sum / valid) : 0;
}

// ==================== RESET TẤT CẢ DỮ LIỆU ====================
void resetAllData() {
  // Reset biến nhịp tim
  irMin = 999999;
  irMax = 0;
  irPrev = 0;
  lastBeatTime = 0;
  bpm = 0;
  avgBPM = 0;
  
  // Reset mảng BPM
  for (int i = 0; i < BPM_SIZE; i++) {
    bpmArray[i] = 0;
  }
  bpmIndex = 0;
  
  // Reset biến SpO2
  redSum = 0;
  irSum = 0;
  sampleCount = 0;
  spo2 = 0;
  
  // Reset mảng dữ liệu
  for (int i = 0; i < MAX_SAMPLES; i++) {
    tempData[i] = 0;
    hrData[i] = 0;
    spo2Data[i] = 0;
  }
  dataIndex = 0;
}

// ==================== ÂM THANH:  SẴN SÀNG ====================
void beepReady() {
  tone(BUZZER_PIN, 1000, 100);
}

// ==================== ÂM THANH: BẮT ĐẦU ĐO ====================
void beepStart() {
  tone(BUZZER_PIN, 1000, 150);
  delay(200);
  tone(BUZZER_PIN, 1500, 150);
}

// ==================== ÂM THANH:  THÀNH CÔNG ====================
void beepSuccess() {
  tone(BUZZER_PIN, 1500, 150);
  delay(200);
  tone(BUZZER_PIN, 1800, 150);
  delay(200);
  tone(BUZZER_PIN, 2000, 300);
}

// ==================== ÂM THANH: CẢNH BÁO ====================
void beepWarning() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000, 200);
    delay(300);
  }
}

// ==================== ÂM THANH: NGUY HIỂM ====================
void beepDanger() {
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, 1500, 100);
    delay(100);
    tone(BUZZER_PIN, 1000, 100);
    delay(100);
  }
}

// ==================== ÂM THANH: LỖI ====================
void beepError() {
  tone(BUZZER_PIN, 500, 500);
}