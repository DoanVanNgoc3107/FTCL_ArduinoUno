/**
 * PROJECT : PHÂN LOẠI SẢN PHẨM THEO MÀU SẮC
 * @Author : TRINH DOAN LINH - MSV
 *           LOC NGUYEN - MSV
 *           DAO TRONG TU - MSV
 * @version 1.2.15
 * @date 12/11/2025
 */

#include <Arduino.h>
#include <Servo.h>

/* ================= CONFIGURATION & PINOUT ================= */

// Định nghĩa chân cho cảm biến màu TCS3200
#define S0_PIN 3
#define S1_PIN 4
#define S2_PIN 5
#define S3_PIN 6
#define OUT_PIN 2

// Định nghĩa chân cho Servo
#define FEEDER_SERVO_PIN 9  // Servo cấp phôi (Servo tầng 1)
#define RAMP_SERVO_PIN 10   // Servo máng trượt (Servo tầng 2)

// Định nghĩa chân nút nhấn
#define BUTTON_PIN 7

// Các vị trí góc của Servo Cấp phôi (Feeder)
#define FEEDER_HOME_POS   0    // Vị trí lỗ hứng phôi từ ống
#define FEEDER_SENSE_POS  90   // Vị trí đưa phôi vào gầm cảm biến
#define FEEDER_DROP_POS   160  // Vị trí nhả phôi xuống máng

// Các vị trí góc của Servo Máng trượt
// TODO: Chỉnh các góc độ để phù hợp với gỉai màu thực tế
#define RAMP_RED_POS    45
#define RAMP_GREEN_POS  90
#define RAMP_BLUE_POS   135
#define RAMP_UNKNOWN_POS 135 // Mặc định nếu không nhận ra màu

enum Color {
    COLOR_UNKNOWN = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
};

/* ================= GLOBAL OBJECTS ================= */
Servo feederServo;
Servo rampServo;

// Biến lưu trạng thái hoạt động của hệ thống
bool isSystemRunning = false;

// Biến lưu giá trị tần số
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

/* ================= FUNCTION PROTOTYPES ================= */
void resetSystem();
void runSortingProcess();
Color detectColor();
void readRawSensorValues();

/* ================= SETUP ================= */
void setup() {
    Serial.begin(9600);
    Serial.println("System Initializing...");

    // Cấu hình chân TCS3200
    pinMode(S0_PIN, OUTPUT);
    pinMode(S1_PIN, OUTPUT);
    pinMode(S2_PIN, OUTPUT);
    pinMode(S3_PIN, OUTPUT);
    pinMode(OUT_PIN, INPUT);

    // Scaling 20%
    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);

    // Cấu hình Servo
    feederServo.attach(FEEDER_SERVO_PIN);
    rampServo.attach(RAMP_SERVO_PIN);

    // Nút nhấn Input Pullup (Trạng thái bình thường là HIGH, nhấn là LOW)
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Reset về vị trí ban đầu
    resetSystem();

    Serial.println("System Ready. Press Button to toggle AUTO MODE.");
}

/* ================= MAIN LOOP ================= */
void loop() {
    // --- XỬ LÝ NÚT NHẤN (BẬT/TẮT CHẾ ĐỘ TỰ ĐỘNG) ---
    static int lastBtnState = HIGH; // Logic ở đây phục vụ cho mục đích bấm 1 lần là chạy, bấm lần nữa là tắt hệ thống
    int currentBtnState = digitalRead(BUTTON_PIN);

    // Kiểm tra sườn xuống (Khi mới nhấn nút)
    if (lastBtnState == HIGH && currentBtnState == LOW) {
        delay(50); // Chống rung (Debounce)
        if (digitalRead(BUTTON_PIN) == LOW) {
            // Đảo trạng thái hoạt động
            isSystemRunning = !isSystemRunning;

            if (isSystemRunning) {
                Serial.println(">>> STARTED: System is now RUNNING automatically.");
            } else {
                Serial.println(">>> STOPPED: System is PAUSED.");
            }
        }
    }
    lastBtnState = currentBtnState;

    // --- XỬ LÝ QUY TRÌNH ---
    // Nếu biến trạng thái là true thì chạy quy trình liên tục
    if (isSystemRunning) {
        runSortingProcess();
        // Sau khi chạy xong 1 quy trình, loop sẽ quay lại
        // và tiếp tục gọi runSortingProcess() nếu isSystemRunning vẫn true.
    }
}

/* ================= FUNCTIONS ================= */

/**
 * @brief Quy trình phân loại 1 sản phẩm
 * Sau khi chạy xong hàm này, hệ thống đã trả về vị trí Home
 * để sẵn sàng đón sản phẩm tiếp theo.
 *
 * TODO: Có thể chỉnh góc độ ở trong phần define cho phù hợp với thực tế
 */
void runSortingProcess() {
    Serial.println("\n--- Processing New Item ---");

    // 1. Cấp phôi: Từ Home (0 độ) mang phôi đến Cảm biến (90 độ)
    // TODO : Thiết kế làm sao cho khi servo lấy phôi thì phôi phải tự động rơi xuống.
    // Lưu ý: Lúc ở Home (trong hàm resetSystem) phôi đã rơi vào lỗ servo
    feederServo.write(FEEDER_SENSE_POS);
    delay(1000); // Đợi servo di chuyển và vật ổn định

    // 2. Đọc màu
    Color detectedColor = detectColor();

    // 3. Điều hướng máng trượt trước
    switch (detectedColor) {
        case COLOR_RED:
            Serial.println("Action: Sort to RED bin");
            rampServo.write(RAMP_RED_POS);
            break;
        case COLOR_GREEN:
            Serial.println("Action: Sort to GREEN bin");
            rampServo.write(RAMP_GREEN_POS);
            break;
        case COLOR_BLUE:
            Serial.println("Action: Sort to BLUE bin");
            rampServo.write(RAMP_BLUE_POS);
            break;
        default:
            Serial.println("Action: Unknown -> Default bin");
            rampServo.write(RAMP_UNKNOWN_POS);
            break;
    }
    delay(500); // Đợi máng xoay xong

    // 4. Nhả phôi: Quay servo trên ra vị trí xả
    feederServo.write(FEEDER_DROP_POS);
    delay(600); // Đợi vật rơi xuống máng

    // 5. Reset: Quay về để hứng viên tiếp theo
    resetSystem();

    // Delay nhẹ để phôi từ ống kịp rơi vào lỗ servo (quan trọng)
    delay(500);
}

/**
 * @brief Đưa servo về vị trí lấy phôi
 */
void resetSystem() {
    // Về vị trí 0 độ để lỗ trên đĩa servo trùng với ống chứa phôi
    feederServo.write(FEEDER_HOME_POS);
    // Không cần reset rampServo để tiết kiệm thời gian, khi nào có màu mới thì tự chuyển cũng được
}

/**
 * @brief Đọc giá trị thô từ cảm biến
 */
void readRawSensorValues() {
    // Đọc RED
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, LOW);
    redFrequency = pulseIn(OUT_PIN, LOW);
    delay(20);

    // Đọc GREEN
    digitalWrite(S2_PIN, HIGH);
    digitalWrite(S3_PIN, HIGH);
    greenFrequency = pulseIn(OUT_PIN, LOW);
    delay(20);

    // Đọc BLUE
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, HIGH);
    blueFrequency = pulseIn(OUT_PIN, LOW);
    delay(20);

    // Debug
    Serial.print("R:"); Serial.print(redFrequency);
    Serial.print(" G:"); Serial.print(greenFrequency);
    Serial.print(" B:"); Serial.println(blueFrequency);
}

/**
 * @brief Logic xác định màu
 * TODO: CẦN CALIBRATE LẠI CÁC THÔNG SỐ Ở ĐÂY DỰA TRÊN THỰC TẾ
 * @return trả ra enum
 */
Color detectColor() {
    readRawSensorValues();

    // Logic so sánh cơ bảns
    // Ví dụ: màu nào có tần số thấp nhất (giá trị nhỏ nhất) là màu đó
    if (redFrequency < greenFrequency && redFrequency < blueFrequency && redFrequency < 200) {
        return COLOR_RED;
    }
    else if (greenFrequency < redFrequency && greenFrequency < blueFrequency && greenFrequency < 200) {
        return COLOR_GREEN;
    }
    else if (blueFrequency < redFrequency && blueFrequency < greenFrequency && blueFrequency < 200) {
        return COLOR_BLUE;
    }
    else {
        return COLOR_UNKNOWN;
    }
}