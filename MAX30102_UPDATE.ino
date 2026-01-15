/*
 * ============================================
 *    HỆ THỐNG ĐO SỨC KHỎE IoT - NHÓM 5
 *    CÁC THÀNH VIÊN:  ĐỖ HOÀN THÀNH
 *                     NGUYỄN ĐỖ ĐÌNH ANH
                       NGUYỄN HOÀI NAM
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
 *    LED Xanh:   (+)->D9 qua 220Ω, (-)->GND
 *    LED Vàng:   (+)->D10 qua 220Ω, (-)->GND
 *    LED Đỏ:    (+)->D11 qua 220Ω, (-)->GND
 * 
 * BẢN CẢI TIẾN (ĐƠN GIẢN HÓA):
 *    - SpO2: Thuật toán AC/DC + Bộ lọc EMA (Exponential Moving Average)
 *    - Nhiệt độ: Lọc Median + EMA để giảm nhiễu
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30105.h"

// ==================== CẤU HÌNH CHÂN ====================
#define LM35_PIN A0
#define BUZZER_PIN 8
#define LED_GREEN_PIN 9
#define LED_YELLOW_PIN 10
#define LED_RED_PIN 11

// ==================== NGƯỠNG CẢNH BÁO ====================
#define HR_WARN_LOW 50
#define HR_WARN_HIGH 120
#define SPO2_WARN 90
#define TEMP_WARN_LOW 35.0
#define TEMP_WARN_HIGH 38.5

#define HR_MIN 60
#define HR_MAX 100
#define SPO2_MIN 95
#define TEMP_MIN 36.0
#define TEMP_MAX 37.5

// ==================== CẤU HÌNH ĐO ====================
#define MEASURE_TIME 20000
#define SAMPLE_RATE 1000
#define PHASE_1_TIME 7000
#define PHASE_2_TIME 14000
#define RESULT_DISPLAY_TIME 10000

// ==================== KHỞI TẠO MODULE ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);
MAX30105 sensor;

// ==================== BIẾN TÍNH NHỊP TIM (GIỮ NGUYÊN) ====================
long irPrev = 0, irMin = 999999, irMax = 0, irThreshold = 0;
unsigned long lastBeatTime = 0;
int bpm = 0, avgBPM = 0;
#define BPM_SIZE 5
int bpmArray[BPM_SIZE];
int bpmIndex = 0;

// ==================== BIẾN TÍNH SPO2 (ĐƠN GIẢN HÓA) ====================
int spo2 = 0;
int spo2Filtered = 95;  // Giá trị lọc EMA
long redSum = 0, irSum = 0;
long redAC = 0, irAC = 0;  // Chỉ lưu max-min
long redMax = 0, redMin = 999999;
long irACMax = 0, irACMin = 999999;
int sampleCount = 0;

// ==================== BIẾN NHIỆT ĐỘ (ĐƠN GIẢN HÓA) ====================
float tempFiltered = 36.0;  // Giá trị lọc EMA

// ==================== BIẾN LƯU TRỮ KẾT QUẢ ====================
#define MAX_SAMPLES 25
float tempData[MAX_SAMPLES];
int hrData[MAX_SAMPLES], spo2Data[MAX_SAMPLES];
int dataIndex = 0;

// ==================== BIẾN TRẠNG THÁI ====================
bool measuring = false, showingResult = false;
unsigned long resultStartTime = 0, startTime = 0, lastSample = 0;
int currentPhase = 0, healthStatus = 0;

// ==================== KÝ TỰ LCD ====================
byte heartChar[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000 };
byte degreeChar[8] = { 0b00110, 0b01001, 0b01001, 0b00110, 0b00000, 0b00000, 0b00000, 0b00000 };
byte progressChar[8] = { 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LM35_PIN, INPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  turnOffAllLeds();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, heartChar);
  lcd.createChar(1, degreeChar);
  lcd.createChar(2, progressChar);

  lcd.clear();
  lcd.print("Health Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Khoi dong.. .");

  testLeds();

  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    lcd.clear();
    lcd.print("LOI SENSOR!");
    Serial.println(F("LOI:  Khong tim thay MAX30102! "));
    while (1) {
      blinkAllLeds();
      tone(BUZZER_PIN, 500, 300);
      delay(500);
    }
  }

  sensor.setup();
  sensor.setPulseAmplitudeRed(0x1F);
  sensor.setPulseAmplitudeIR(0x1F);

  Serial.println(F("\n=== HE THONG DO SUC KHOE IoT ===\n"));

  delay(1000);
  showReady();
  tone(BUZZER_PIN, 1000, 100);
}

// ==================== LED FUNCTIONS ====================
void testLeds() {
  digitalWrite(LED_GREEN_PIN, HIGH);
  tone(BUZZER_PIN, 1000, 100);
  delay(300);
  digitalWrite(LED_YELLOW_PIN, HIGH);
  tone(BUZZER_PIN, 1200, 100);
  delay(300);
  digitalWrite(LED_RED_PIN, HIGH);
  tone(BUZZER_PIN, 1500, 100);
  delay(300);
  turnOffAllLeds();
  delay(200);
}

void blinkAllLeds() {
  static bool s = false;
  s = !s;
  digitalWrite(LED_GREEN_PIN, s);
  digitalWrite(LED_YELLOW_PIN, s);
  digitalWrite(LED_RED_PIN, s);
}

void turnOffAllLeds() {
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
}

void updateProgressLeds(unsigned long elapsed) {
  int newPhase = (elapsed < PHASE_1_TIME) ? 1 : (elapsed < PHASE_2_TIME) ? 2
                                                                         : 3;

  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_YELLOW_PIN, newPhase >= 2);
  digitalWrite(LED_RED_PIN, newPhase >= 3);

  if (newPhase != currentPhase) {
    currentPhase = newPhase;
    tone(BUZZER_PIN, 1000 + currentPhase * 200, 100);
    Serial.print(F(">> Giai doan "));
    Serial.print(currentPhase);
    Serial.println(F("/3"));
  }
}

void blinkLed(int pin, int interval) {
  static unsigned long lastBlink = 0;
  static bool state = false;
  if (millis() - lastBlink > interval) {
    state = !state;
    digitalWrite(pin, state);
    if (pin == LED_RED_PIN && state) tone(BUZZER_PIN, 1000, 50);
    lastBlink = millis();
  }
}

// ==================== HEALTH STATUS ====================
int calculateHealthStatus(float temp, int hr, int sp) {
  // Kiểm tra mức ĐỎ
  if ((temp > 0 && (temp < TEMP_WARN_LOW || temp > TEMP_WARN_HIGH)) || (hr > 0 && (hr < HR_WARN_LOW || hr > HR_WARN_HIGH)) || (sp > 0 && sp < SPO2_WARN)) {
    return 2;
  }
  // Kiểm tra mức VÀNG
  if ((temp > 0 && (temp < TEMP_MIN || temp > TEMP_MAX)) || (hr > 0 && (hr < HR_MIN || hr > HR_MAX)) || (sp > 0 && sp < SPO2_MIN)) {
    return 1;
  }
  return 0;
}

// ==================== MAIN LOOP ====================
void loop() {
  if (showingResult) {
    if (healthStatus == 2) blinkLed(LED_RED_PIN, 200);
    else if (healthStatus == 1) blinkLed(LED_YELLOW_PIN, 400);

    if (millis() - resultStartTime > RESULT_DISPLAY_TIME) {
      showingResult = false;
      healthStatus = 0;
      showReady();
    }
    delay(10);
    return;
  }

  long irValue = sensor.getIR();
  bool fingerDetected = (irValue > 50000);

  if (fingerDetected && !measuring) startMeasurement();

  // Hủy đo nếu mất tín hiệu > 2 giây
  static unsigned long lostTime = 0;
  if (!fingerDetected && measuring) {
    if (lostTime == 0) lostTime = millis();
    else if (millis() - lostTime > 2000) {
      lostTime = 0;
      cancelMeasurement();
      return;
    }
  } else lostTime = 0;

  if (measuring) {
    unsigned long elapsed = millis() - startTime;

    if (elapsed < MEASURE_TIME) {
      long redValue = sensor.getRed();
      calculateHeartRate(irValue);
      calculateSpO2(irValue, redValue);

      if (millis() - lastSample >= SAMPLE_RATE) {
        collectSample();
        lastSample = millis();
      }

      updateProgressLeds(elapsed);
      updateDisplay(elapsed);
    } else {
      finishMeasurement();
    }
  }

  delay(10);
}

// ==================== MEASUREMENT FUNCTIONS ====================
void startMeasurement() {
  measuring = true;
  showingResult = false;
  startTime = millis();
  lastSample = millis();
  dataIndex = 0;
  currentPhase = 0;

  resetAllData();
  turnOffAllLeds();

  lcd.clear();
  lcd.print("Bat dau do...");
  lcd.setCursor(0, 1);
  lcd.print("Giu yen ngon tay");

  Serial.println(F("\n>>> BAT DAU DO <<<"));
  Serial.println(F("Time,Temp,HR,SpO2"));

  tone(BUZZER_PIN, 1000, 150);
  delay(200);
  tone(BUZZER_PIN, 1500, 150);
  delay(300);
}

void collectSample() {
  if (dataIndex >= MAX_SAMPLES) return;

  float temp = readTemperature();
  tempData[dataIndex] = temp;
  hrData[dataIndex] = avgBPM;
  spo2Data[dataIndex] = spo2;
  dataIndex++;

  Serial.print(millis() - startTime);
  Serial.print(",");
  Serial.print(temp, 1);
  Serial.print(",");
  Serial.print(avgBPM);
  Serial.print(",");
  Serial.println(spo2);
}

// ==================== ĐỌC NHIỆT ĐỘ (ĐƠN GIẢN - EMA FILTER) ====================
float readTemperature() {
  // Đọc và lọc median đơn giản (5 mẫu)
  int readings[5];
  for (int i = 0; i < 5; i++) {
    readings[i] = analogRead(LM35_PIN);
    delay(1);
  }

  // Bubble sort đơn giản
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[i] > readings[j]) {
        int t = readings[i];
        readings[i] = readings[j];
        readings[j] = t;
      }
    }
  }

  // Lấy giá trị giữa (median)
  float tempC = (readings[2] * 500.0) / 1024.0;

  // Kiểm tra hợp lệ (25-45°C)
  if (tempC < 25.0 || tempC > 45.0) {
    return tempFiltered;  // Giữ giá trị cũ
  }

  // Lọc EMA (Exponential Moving Average) - RẤT ĐƠN GIẢN
  tempFiltered = 0.7 * tempFiltered + 0.3 * tempC;

  return tempFiltered;
}

// ==================== TÍNH NHỊP TIM (GIỮ NGUYÊN 100%) ====================
void calculateHeartRate(long irValue) {
  if (irValue < irMin) irMin = irValue;
  if (irValue > irMax) irMax = irValue;

  irThreshold = (irMin + irMax) / 2;

  if (irValue > irThreshold && irPrev <= irThreshold) {
    unsigned long now = millis();

    if (lastBeatTime > 0) {
      unsigned long interval = now - lastBeatTime;

      if (interval > 300 && interval < 1500) {
        bpm = 60000 / interval;

        if (bpm >= 40 && bpm <= 200) {
          bpmArray[bpmIndex] = bpm;
          bpmIndex = (bpmIndex + 1) % BPM_SIZE;

          int sum = 0, count = 0;
          for (int i = 0; i < BPM_SIZE; i++) {
            if (bpmArray[i] > 0) {
              sum += bpmArray[i];
              count++;
            }
          }
          if (count > 0) avgBPM = sum / count;
        }
      }
    }
    lastBeatTime = now;
  }

  irPrev = irValue;

  static unsigned long lastReset = 0;
  if (millis() - lastReset > 3000) {
    irMin = irValue - 1000;
    irMax = irValue + 1000;
    lastReset = millis();
  }
}

// ==================== TÍNH SPO2 (ĐƠN GIẢN HÓA) ====================
void calculateSpO2(long irValue, long redValue) {
  // Tích lũy DC
  redSum += redValue;
  irSum += irValue;

  // Tìm AC (min/max)
  if (redValue > redMax) redMax = redValue;
  if (redValue < redMin) redMin = redValue;
  if (irValue > irACMax) irACMax = irValue;
  if (irValue < irACMin) irACMin = irValue;

  sampleCount++;

  // Tính mỗi 100 mẫu
  if (sampleCount >= 100) {
    long redDC = redSum / sampleCount;
    long irDC = irSum / sampleCount;
    long redACVal = redMax - redMin;
    long irACVal = irACMax - irACMin;

    // Kiểm tra tín hiệu hợp lệ
    if (irDC > 50000 && redDC > 10000 && irACVal > 100 && redACVal > 100) {
      // Tính R = (redAC/redDC) / (irAC/irDC)
      float R = ((float)redACVal / redDC) / ((float)irACVal / irDC);

      // Công thức đơn giản đã calibrate
      int rawSpO2;
      if (R < 0.4) rawSpO2 = 99;
      else if (R > 1.0) rawSpO2 = 80;
      else rawSpO2 = (int)(104.0 - 17.0 * R);

      // Lọc EMA - CHỈ 1 DÒNG!
      spo2Filtered = (spo2Filtered * 7 + rawSpO2 * 3) / 10;
      spo2 = constrain(spo2Filtered, 70, 100);
    }

    // Reset
    redSum = 0;
    irSum = 0;
    sampleCount = 0;
    redMax = 0;
    redMin = 999999;
    irACMax = 0;
    irACMin = 999999;
  }
}

// ==================== DISPLAY ====================
void updateDisplay(unsigned long elapsed) {
  float currentTemp = (dataIndex > 0) ? tempData[dataIndex - 1] : tempFiltered;

  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(currentTemp, 1);
  lcd.write(1);
  lcd.print(" ");
  lcd.write(byte(0));
  lcd.print(":");
  if (avgBPM < 100) lcd.print(" ");
  if (avgBPM < 10) lcd.print(" ");
  lcd.print(avgBPM);

  lcd.setCursor(0, 1);
  lcd.print("O2:");
  if (spo2 < 100) lcd.print(" ");
  lcd.print(spo2);
  lcd.print("% ");

  int progress = map(elapsed, 0, MEASURE_TIME, 0, 6);
  lcd.print("[");
  for (int i = 0; i < 6; i++) lcd.print(i < progress ? (char)0xFF : '-');
  lcd.print("]");
}

// ==================== FINISH/CANCEL ====================
void finishMeasurement() {
  measuring = false;

  float avgTemp = calcAvg(tempData, dataIndex);
  int avgHR = calcAvgInt(hrData, dataIndex);
  int avgSpO2 = calcAvgInt(spo2Data, dataIndex);

  healthStatus = calculateHealthStatus(avgTemp, avgHR, avgSpO2);
  showingResult = true;
  resultStartTime = millis();

  turnOffAllLeds();
  if (healthStatus == 0) digitalWrite(LED_GREEN_PIN, HIGH);

  lcd.clear();
  lcd.print("T:");
  lcd.print(avgTemp, 1);
  lcd.write(1);
  lcd.print(" ");
  lcd.write(byte(0));
  lcd.print(": ");
  lcd.print(avgHR);
  lcd.setCursor(0, 1);
  lcd.print("O2:");
  lcd.print(avgSpO2);
  lcd.print("% ");
  lcd.print(healthStatus == 0 ? "[OK]" : healthStatus == 1 ? "[WARN]"
                                                           : "[!!! ]");

  Serial.println(F("\n=== KET QUA ==="));
  Serial.print(F("Temp:  "));
  Serial.print(avgTemp, 1);
  Serial.println(F("C"));
  Serial.print(F("HR: "));
  Serial.print(avgHR);
  Serial.println(F(" BPM"));
  Serial.print(F("SpO2: "));
  Serial.print(avgSpO2);
  Serial.println(F("%"));

  // Beep theo kết quả
  if (healthStatus == 0) {
    tone(BUZZER_PIN, 1500, 150);
    delay(200);
    tone(BUZZER_PIN, 2000, 300);
  } else if (healthStatus == 1) {
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1000, 200);
      delay(300);
    }
  } else {
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 1500, 100);
      delay(100);
      tone(BUZZER_PIN, 1000, 100);
      delay(100);
    }
  }
}

void cancelMeasurement() {
  measuring = false;
  showingResult = false;
  turnOffAllLeds();

  lcd.clear();
  lcd.print("DA HUY DO!");
  lcd.setCursor(0, 1);
  lcd.print("Giu yen ngon tay");

  Serial.println(F("\n! !! DA HUY !!!"));
  tone(BUZZER_PIN, 500, 500);
  delay(2000);
  showReady();
}

void showReady() {
  turnOffAllLeds();
  digitalWrite(LED_GREEN_PIN, HIGH);
  lcd.clear();
  lcd.print("  SAN SANG DO");
  lcd.setCursor(0, 1);
  lcd.print("Dat ngon tay.. .");
  Serial.println(F("San sang... "));
}

// ==================== HELPER FUNCTIONS ====================
float calcAvg(float arr[], int count) {
  if (count == 0) return 0;
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < count; i++)
    if (arr[i] > 0) {
      sum += arr[i];
      valid++;
    }
  return valid > 0 ? sum / valid : 0;
}

int calcAvgInt(int arr[], int count) {
  if (count == 0) return 0;
  long sum = 0;
  int valid = 0;
  for (int i = 0; i < count; i++)
    if (arr[i] > 0) {
      sum += arr[i];
      valid++;
    }
  return valid > 0 ? sum / valid : 0;
}

void resetAllData() {
  irMin = 999999;
  irMax = 0;
  irPrev = 0;
  lastBeatTime = 0;
  bpm = 0;
  avgBPM = 0;
  bpmIndex = 0;
  for (int i = 0; i < BPM_SIZE; i++) bpmArray[i] = 0;

  redSum = 0;
  irSum = 0;
  sampleCount = 0;
  spo2 = 0;
  redMax = 0;
  redMin = 999999;
  irACMax = 0;
  irACMin = 999999;

  for (int i = 0; i < MAX_SAMPLES; i++) {
    tempData[i] = 0;
    hrData[i] = 0;
    spo2Data[i] = 0;
  }
  dataIndex = 0;
}
