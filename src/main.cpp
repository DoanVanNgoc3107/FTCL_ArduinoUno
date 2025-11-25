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
#define FEEDER_SERVO_PIN 9  // Servo cấp phôi (Servo trên)
#define RAMP_SERVO_PIN 10   // Servo máng trượt (Servo dưới)

// Định nghĩa chân nút nhấn
#define BUTTON_PIN 7

// Các vị trí góc của Servo Cấp phôi (Feeder) - Cần chỉnh thực tếs
#define FEEDER_HOME_POS   0    // Vị trí lấy phôi từ ống
#define FEEDER_SENSE_POS  90   // Vị trí đưa phôi vào gầm cảm biến
#define FEEDER_DROP_POS   160  // Vị trí nhả phôi xuống máng

// Các vị trí góc của Servo Máng trượt - Tương ứng màu
#define RAMP_RED_POS    45
#define RAMP_GREEN_POS  90
#define RAMP_BLUE_POS   135

bool lastButtonState = LOW;

// Define thêm vị trí cho màu
enum Color
{
    COLOR_UNKNOWN = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
};

// ==== Định nghĩa các function ====
void resetSystem(); // ham nay

void runSortingProcess();

Color detectColor();

bool getStateBtn(int const btnPin);

/* ================= GLOBAL OBJECTS ================= */
Servo feederServo; // con này để lấy phôi từ ống xuống nhé (chân 9)
Servo rampServo; // con này để di chuyển máng trượt (chân 10)

// Biến lưu giá trị tần số đọc được
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

/* ================= SETUP ================= */
void setup()
{
    // Cấu hình serial (viết phần này thành function ra ngoài để dùng cũng được)
    Serial.begin(9600);
    Serial.println("System Initializing...");

    // Cấu hình chân TCS3200
    pinMode(S0_PIN, OUTPUT);
    pinMode(S1_PIN, OUTPUT);
    pinMode(S2_PIN, OUTPUT);
    pinMode(S3_PIN, OUTPUT);
    pinMode(OUT_PIN, INPUT);

    // Cài đặt tần số tỉ lệ 20% (Phù hợp với Arduino)
    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);

    // Cấu hình Servo. Method attach để gán chân servo
    feederServo.attach(FEEDER_SERVO_PIN);
    rampServo.attach(RAMP_SERVO_PIN);

    // Cấu hình nút nhấn (Input Pullup để không cần điện trở ngoài)
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Đưa hệ thống về trạng thái ban đầu
    resetSystem();

    Serial.println("System Ready. Press Button to Start.");
}

/* ================= MAIN LOOP ================= */
void loop()
{
    delay(50); // Chống rung phím (nếu phát triền hơn thì có thể viết chống dội)
    runSortingProcess();
    delay(500); // Delay giữa các lần phân loại
}

/* ================= FUNCTIONS ================= */

/**
 * @brief Hàm này đọc trạng thái của btn
 * @param btnPin chân của btn
 * @return HIGH hoặc LOW
 */
bool getStateBtn(int const btnPin)
{
    return digitalRead(btnPin);
}

/**
 * @brief Quy trình phân loại chính
 * Luồng hoạt động như sau: ...
 */
void runSortingProcess()
{
    Serial.println("Bat dau quy trinh phan loai");

    // Step 1 - Cấp phôi: Di chuyển từ chỗ lấy đến chỗ cảm biến
    feederServo.write(FEEDER_SENSE_POS);
    delay(1000); // Đợi servo chạy và vật ổn định trước khi đọc

    // Step 2 -s Đọc màu
    Color detectedColor = detectColor();

    // Step 3 - Điều khiển máng phân loại (Servo máng) dựa trên màu
    switch (detectedColor)
    {
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
        Serial.println("Action: Unknown Color - Check Serial Monitor");
        // Có thể cho về một ô mặc định hoặc giữ nguyên
        break;
    }
    delay(500); // Đợi máng trượt vào vị trí

    // 4. Nhả phôi: Quay thêm 1 góc nhỏ để vật rơi
    feederServo.write(FEEDER_DROP_POS);
    delay(500); // Đợi vật rơi hẳn

    // 5. Reset: Quay về vị trí lấy phôi
    resetSystem();

    Serial.println("<<< Process Finished");
}

/**
 * @brief Đưa servo về vị trí mặc định
 */
void resetSystem()
{
    feederServo.write(FEEDER_HOME_POS);
    // ở đây giữ nguyên vị trí cũ để tiết kiệm thời gian
}

/**
 * @brief Đọc giá trị thô từ cảm biến TCS3200
 * @details Thay đổi S2, S3 để chọn bộ lọc màu
 */
void readRawSensorValues()
{
    // Đọc RED (S2 Low, S3 Low)
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, LOW);
    redFrequency = pulseIn(OUT_PIN, LOW);
    delay(20); // Delay nhỏ để cảm biến ổn định

    // Đọc GREEN (S2 High, S3 High)
    digitalWrite(S2_PIN, HIGH);
    digitalWrite(S3_PIN, HIGH);
    greenFrequency = pulseIn(OUT_PIN, LOW);
    delay(20);

    // Đọc BLUE (S2 Low, S3 High)
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, HIGH);
    blueFrequency = pulseIn(OUT_PIN, LOW);
    delay(20);

    // In giá trị ra Serial để Cân chỉnh (Rất quan trọng)
    Serial.print("R=");
    Serial.print(redFrequency);
    Serial.print(" G=");
    Serial.print(greenFrequency);
    Serial.print(" B=");
    Serial.print(blueFrequency);
}

/**
 * @brief Xác định màu sắc dựa trên tần số
 * @return Color enum
 */
Color detectColor()
{
    readRawSensorValues();

    // CHÚ Ý: pulseIn trả về chu kỳ (thời gian).
    // Giá trị càng NHỎ nghĩa là màu đó càng MẠNH.

    // Logic phát hiện màu ĐỎ
    // Thường R sẽ nhỏ nhất và nhỏ hơn 1 ngưỡng nào đó
    if (redFrequency < greenFrequency && redFrequency < blueFrequency && redFrequency < 100)
    {
        Serial.println(" -> Detected: RED");
        return COLOR_RED;
    }

    // Logic phát hiện màu XANH LÁ
    else if (greenFrequency < redFrequency && greenFrequency < blueFrequency && greenFrequency < 120)
    {
        Serial.println(" -> Detected: GREEN");
        return COLOR_GREEN;
    }

    // Logic phát hiện màu XANH DƯƠNG
    // Nếu đề tài yêu cầu màu Blue
    else if (blueFrequency < redFrequency && blueFrequency < greenFrequency)
    {
        Serial.println(" -> Detected: BLUE");
        return COLOR_BLUE;
    }

    else
    {
        Serial.println(" -> Unknown");
        return COLOR_UNKNOWN;
    }
}
