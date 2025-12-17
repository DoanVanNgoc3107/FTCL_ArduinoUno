#include <Servo.h>
#include <Arduino.h>

/* ================= CONFIGURATION & PINOUT ================= */

#define LCD_ADR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Baud cua serial
#define BAUD_RATE 9600

// Cảm biến màu TCS3200
#define S0_PIN 3 // Chan SO noi voi chan 3 cua arduino
#define S1_PIN 4
#define S2_PIN 5
#define S3_PIN 6
#define OUT_PIN 2

// Servo
#define FEEDER_SERVO_PIN 9 // Chan lay phoi chan 9
#define RAMP_SERVO_PIN 10 // chan mang chan 10

// Button
#define BUTTON_PIN 7 // Chan nut khoi dong

// Vị trí Servo (Cần tinh chỉnh thực tế)
#define FEEDER_HOME_POS   81    // Lỗ hứng phôi
#define FEEDER_SENSE_POS  45   // Đưa phôi vào gầm cảm biến (chỉnh 90->95 để cân giữa)
#define FEEDER_DROP_POS   8  // Nhả phôi (tăng lên 170 để chắc chắn rớt)

// Vị trí Máng trượt
#define RAMP_RED_POS    50
#define RAMP_GREEN_POS  90
#define RAMP_BLUE_POS   130
#define RAMP_UNKNOWN_POS 140

/* ================= GLOBAL OBJECTS ================= */
Servo feederServo; // Khoi tao servo lay phoi
Servo rampServo;  // Khoi tao servo mang truot

bool isSystemRunning = false;

// Dùng unsigned long cho pulseIn
unsigned long redFreq = 0;
unsigned long greenFreq = 0;
unsigned long blueFreq = 0;

// function interface
void resetSystem(); // dat lai he thong
void runSortingProcess(); // Qua trinh sap xep

/* ================= SETUP ================= */
void setup() {
    Serial.begin(BAUD_RATE); // vd ngon ngu giao tiep thong nhat cho arduino va lap

    pinMode(S0_PIN, OUTPUT);
    pinMode(S1_PIN, OUTPUT);
    pinMode(S2_PIN, OUTPUT);
    pinMode(S3_PIN, OUTPUT);
    pinMode(OUT_PIN, INPUT);

    // Scaling 20% (Phổ biến nhất cho Arduino Uno)
    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);

    // Set up chan Set up cua Servo de nhan hoac truyen tin hieu
    feederServo.attach(FEEDER_SERVO_PIN);
    rampServo.attach(RAMP_SERVO_PIN);

    pinMode(BUTTON_PIN, INPUT_PULLUP); // Input pullub chi dung cho nut bam

    // Về vị trí ban đầu
    resetSystem();

    Serial.println("HE THONG SAN SANG.");
    Serial.println("Nhan nut de BAT/TAT che do tu dong.");
}

/* ================= MAIN LOOP ================= */
void loop() {
    // Xử lý nút nhấn (Bấm 1 lần để bật, bấm lần nữa để tắt)
    static int lastBtnState = HIGH;
    int currentBtnState = digitalRead(BUTTON_PIN);

    if (lastBtnState == HIGH && currentBtnState == LOW) {
        delay(50); // Chống rung
        if (digitalRead(BUTTON_PIN) == LOW) {
            isSystemRunning = !isSystemRunning;
            if (isSystemRunning) Serial.println(">>> START: HỆ THỐNG CHẠY");
            else Serial.println(">>> STOP: HỆ THỐNG DỪNG (Sẽ dừng sau khi xong viên hiện tại)");
        }
    }
    lastBtnState = currentBtnState;

    if (isSystemRunning) {
        runSortingProcess();
    }
}

/* ================= FUNCTIONS ================= */

/**
 * Đưa servo lấy phôi về vị trí nhận phôi
 */
void resetSystem() {
    feederServo.write(FEEDER_HOME_POS);
}

void readColorSensor() {
    // Đọc RED
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, LOW);
    redFreq = pulseIn(OUT_PIN, LOW);
    delay(10); // Delay nhỏ để cảm biến ổn định

    // Đọc GREEN
    digitalWrite(S2_PIN, HIGH);
    digitalWrite(S3_PIN, HIGH);
    greenFreq = pulseIn(OUT_PIN, LOW);
    delay(10);

    // Đọc BLUE
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, HIGH);
    blueFreq = pulseIn(OUT_PIN, LOW);
    delay(10);

    // In ra Serial để bạn hiệu chỉnh (Calibration)
    Serial.print("R:"); Serial.print(redFreq);
    Serial.print(" G:"); Serial.print(greenFreq);
    Serial.print(" B:"); Serial.print(blueFreq);
}

/**
 *
 * @return
 */
int detectColor() {
    readColorSensor();

    if (redFreq < greenFreq && redFreq < blueFreq && redFreq < 80) { // Thêm điều kiện < 80 để chắc chắn là màu đậm
        Serial.println(" -> DO (RED)");
        return 1;
    }
    else if (greenFreq < redFreq && greenFreq < blueFreq && greenFreq < 90) {
        Serial.println(" -> XANH LA (GREEN)");
        return 2;
    }
    else if (blueFreq < redFreq && blueFreq < greenFreq && blueFreq < 80) {
        Serial.println(" -> XANH DUONG (BLUE)");
        return 3;
    }
}

void runSortingProcess() {
    // 1. Đưa phôi vào cảm biến
    feederServo.write(FEEDER_SENSE_POS);
    delay(800); // Đợi servo chạy tới nơi

    // 2. Đọc màu nhiều lần để chính xác hơn (Lấy trung bình hoặc đọc 2 lần)
    // Ở đây mình đọc 1 lần cho đơn giản code
    int colorCode = detectColor();

    // 3. Xoay máng trượt
    switch (colorCode) {
        case 1: rampServo.write(RAMP_RED_POS); break;
        case 2: rampServo.write(RAMP_GREEN_POS); break;
        case 3: rampServo.write(RAMP_BLUE_POS); break;
        default: ;
    }
    delay(400); // Đợi máng xoay

    // 4. Nhả phôi
    feederServo.write(FEEDER_DROP_POS);
    delay(500); // Đợi phôi rớt

    // 5. Quay về đón phôi mới
    resetSystem();
    delay(600); // QUAN TRỌNG: Đợi phôi từ ống rớt xuống lỗ servo
}