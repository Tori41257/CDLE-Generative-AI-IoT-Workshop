#include <Arduino.h>
#include <M5Unified.h>
#include <M5Chain.h>
// ============================================================
// M5Stack Core2 HY2.0-4P port
//
// 黄：GPIO32 = Core2 TX → Chain RX
// 白：GPIO33 = Core2 RX ← Chain TX
// ============================================================
constexpr gpio_num_t CHAIN_RX_PIN = GPIO_NUM_33;
constexpr gpio_num_t CHAIN_TX_PIN = GPIO_NUM_32;
constexpr uint32_t CHAIN_BAUDRATE = 115200;
// 5秒間回転がなければアラート
constexpr uint32_t STOP_TIMEOUT_MS = 5000;
// 距離取得間隔
constexpr uint32_t TOF_INTERVAL_MS = 200;
// アラート設定
constexpr uint32_t ALERT_FREQUENCY_HZ = 2000;
constexpr uint32_t ALERT_DURATION_MS = 1000;
// 音量：0～255
constexpr uint8_t SPEAKER_VOLUME = 50;
constexpr uint16_t MAX_CHAIN_DEVICES = 8;
Chain M5Chain;
device_info_t deviceBuffer[MAX_CHAIN_DEVICES];
device_list_t deviceList;
uint16_t deviceCount = 0;
uint16_t encoderId = 0;
uint16_t tofId = 0;
int16_t encoderValue = 0;
int16_t previousEncoderValue = INT16_MIN;
uint16_t tofDistance = 0;
uint16_t previousTofDistance = UINT16_MAX;
uint8_t operationStatus = 0;
bool encoderReady = false;
bool tofReady = false;
bool alertActive = false;
uint32_t lastRotationTime = 0;
uint32_t lastTofReadTime = 0;
// ------------------------------------------------------------
// 中央揃え表示
// ------------------------------------------------------------
void drawCenteredText(const String& text, int32_t y) {
    int32_t x =
        (M5.Display.width() - M5.Display.textWidth(text)) / 2;
if (x < 0) {
    x = 0;
}

M5.Display.setCursor(x, y);
M5.Display.print(text);
}
// ------------------------------------------------------------
// 起動画面
// ------------------------------------------------------------
void drawStartupScreen() {
    M5.Display.fillScreen(TFT_BLACK);
M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
M5.Display.setTextSize(2);
drawCenteredText("CHAIN DEVICES", 35);

M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
drawCenteredText("Connecting...", 105);

M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
M5.Display.setTextSize(1);
drawCenteredText("Encoder + ToF", 165);
drawCenteredText("RX:G33  TX:G32", 190);
}
// ------------------------------------------------------------
// エラー画面
// ------------------------------------------------------------
void drawErrorScreen(const String& message) {
    M5.Display.fillScreen(TFT_RED);
M5.Display.setTextColor(TFT_WHITE, TFT_RED);
M5.Display.setTextSize(2);
drawCenteredText("DEVICE ERROR", 35);

M5.Display.setTextSize(1);
drawCenteredText(message, 100);
drawCenteredText("Check Chain direction", 145);
drawCenteredText("Then restart Core2", 170);
}
// ------------------------------------------------------------
// 通常画面
// ------------------------------------------------------------
void drawMainScreen() {
    M5.Display.fillScreen(TFT_BLACK);
M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
M5.Display.setTextSize(2);
drawCenteredText("ROTATION COUNT", 8);

M5.Display.drawFastHLine(
    20,
    38,
    M5.Display.width() - 40,
    TFT_DARKGREY
);

M5.Display.drawFastHLine(
    20,
    122,
    M5.Display.width() - 40,
    TFT_DARKGREY
);

M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
M5.Display.setTextSize(1);
drawCenteredText("DISTANCE", 132);
drawCenteredText("Press knob to reset count", 218);
}
// ------------------------------------------------------------
// カウント表示
// ------------------------------------------------------------
void drawEncoderValue() {
    M5.Display.fillRect(
        0,
        45,
        M5.Display.width(),
        70,
        TFT_BLACK
    );
if (encoderValue > 0) {
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
} else if (encoderValue < 0) {
    M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
} else {
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
}

M5.Display.setTextSize(5);
drawCenteredText(String(encoderValue), 62);
}
// ------------------------------------------------------------
// 距離表示
// ------------------------------------------------------------
void drawDistanceValue() {
    M5.Display.fillRect(
        0,
        150,
        M5.Display.width(),
        55,
        TFT_BLACK
    );
M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
M5.Display.setTextSize(3);

// 測定値を範囲判定せず、そのまま表示
drawCenteredText(
    String(tofDistance) + " mm",
    160
);
}
// ------------------------------------------------------------
// アラート音
// ------------------------------------------------------------
void startAlert() {
    if (alertActive) {
        return;
    }
alertActive = true;

Serial.println("STOP ALERT");

// 画面は変更しない
M5.Speaker.tone(
    ALERT_FREQUENCY_HZ,
    ALERT_DURATION_MS
);
}
// ------------------------------------------------------------
// 回転再開
// ------------------------------------------------------------
void clearAlert() {
    if (!alertActive) {
        return;
    }
alertActive = false;

// アラート音が再生中なら停止
M5.Speaker.stop();

Serial.println("Rotation resumed");
}
// ------------------------------------------------------------
// Chainデバイスを検索
// ------------------------------------------------------------
bool findChainDevices() {
    chain_status_t status;
status = M5Chain.getDeviceNum(&deviceCount);

if (status != CHAIN_OK) {
    Serial.print("getDeviceNum failed: ");
    Serial.println(status);
    return false;
}

Serial.print("Chain device count: ");
Serial.println(deviceCount);

if (deviceCount == 0 ||
    deviceCount > MAX_CHAIN_DEVICES) {
    return false;
}

deviceList.count = deviceCount;
deviceList.devices = deviceBuffer;

if (!M5Chain.getDeviceList(&deviceList)) {
    Serial.println("getDeviceList failed");
    return false;
}

encoderId = 0;
tofId = 0;

for (uint16_t i = 0; i < deviceList.count; i++) {
    uint16_t id = deviceList.devices[i].id;
    chain_device_type_t type =
        deviceList.devices[i].device_type;

    Serial.print("Device ID: ");
    Serial.print(id);
    Serial.print(" / Type: ");
    Serial.println(type);

    if (type == CHAIN_ENCODER_TYPE_CODE) {
        encoderId = id;

        Serial.print("Encoder found. ID: ");
        Serial.println(encoderId);
    }

    if (type == CHAIN_TOF_TYPE_CODE) {
        tofId = id;

        Serial.print("ToF found. ID: ");
        Serial.println(tofId);
    }
}

encoderReady = encoderId != 0;
tofReady = tofId != 0;

return encoderReady && tofReady;
}
// ------------------------------------------------------------
// Encoder初期化
// ------------------------------------------------------------
bool setupEncoder() {
    chain_status_t status;
status = M5Chain.setEncoderABDirect(
    encoderId,
    ENCODER_AB,
    &operationStatus
);

if (status != CHAIN_OK) {
    Serial.println("Encoder direction setting failed");
    return false;
}

status = M5Chain.setEncoderButtonTriggerInterval(
    encoderId,
    BUTTON_DOUBLE_CLICK_TIME_500MS,
    BUTTON_LONG_PRESS_TIME_5S,
    &operationStatus
);

if (status != CHAIN_OK) {
    Serial.println("Encoder button setting failed");
    return false;
}

status = M5Chain.getEncoderValue(
    encoderId,
    &encoderValue
);

if (status != CHAIN_OK) {
    Serial.println("Encoder initial read failed");
    return false;
}

previousEncoderValue = encoderValue;

return true;
}
// ------------------------------------------------------------
// ToF初期化
// ------------------------------------------------------------
bool setupToF() {
    chain_status_t status;
// 連続測定モードに変更
status = M5Chain.setToFMeasureMode(
    tofId,
    CHAIN_TOF_MODE_CONTINUOUS,
    &operationStatus
);

if (status != CHAIN_OK) {
    Serial.print("ToF mode setting failed: ");
    Serial.println(status);
    return false;
}

Serial.print("ToF mode operation status: ");
Serial.println(operationStatus);

// 測定時間：50ms
status = M5Chain.setToFMeasureTime(
    tofId,
    50,
    &operationStatus
);

if (status != CHAIN_OK) {
    Serial.print("ToF measure time setting failed: ");
    Serial.println(status);
    return false;
}

// 連続測定の開始を待つ
delay(500);

status = M5Chain.getToFDistance(
    tofId,
    &tofDistance
);

if (status != CHAIN_OK) {
    Serial.print("ToF initial read failed: ");
    Serial.println(status);
    return false;
}

// 最初のloopでも必ず更新する
previousTofDistance = UINT16_MAX;

Serial.print("Initial distance: ");
Serial.print(tofDistance);
Serial.println(" mm");

return true;
}
// ------------------------------------------------------------
// 初期化
// ------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
auto config = M5.config();
config.internal_spk = true;
M5.begin(config);

M5.Display.setRotation(1);
M5.Display.setBrightness(180);

M5.Speaker.begin();
M5.Speaker.setVolume(SPEAKER_VOLUME);

drawStartupScreen();

M5Chain.begin(
    &Serial2,
    CHAIN_BAUDRATE,
    CHAIN_RX_PIN,
    CHAIN_TX_PIN
);

Serial.println();
Serial.println("==========================");
Serial.println("M5Stack Core2");
Serial.println("Chain Encoder + Chain ToF");
Serial.println("HY2.0 RX=GPIO33 TX=GPIO32");
Serial.println("==========================");

uint32_t startTime = millis();

// 最大10秒間、接続を待つ
while (!M5Chain.isDeviceConnected()) {
    if (millis() - startTime >= 10000) {
        drawErrorScreen("No Chain device found");
        return;
    }

    delay(200);
}

// デイジーチェーン認識待ち
delay(1500);

if (!findChainDevices()) {
    if (!encoderReady) {
        drawErrorScreen("Encoder not found");
    } else {
        drawErrorScreen("Chain ToF not found");
    }

    return;
}

if (!setupEncoder()) {
    drawErrorScreen("Encoder setup failed");
    return;
}

if (!setupToF()) {
    drawErrorScreen("ToF setup failed");
    return;
}

lastRotationTime = millis();
lastTofReadTime = millis();

drawMainScreen();
drawEncoderValue();
drawDistanceValue();
}
// ------------------------------------------------------------
// メインループ
// ------------------------------------------------------------
void loop() {
    M5.update();
if (!encoderReady || !tofReady) {
    delay(100);
    return;
}

chain_status_t status;

// ========================================================
// エンコーダー取得
// ========================================================
status = M5Chain.getEncoderValue(
    encoderId,
    &encoderValue
);

if (status == CHAIN_OK) {
    if (encoderValue != previousEncoderValue) {
        previousEncoderValue = encoderValue;
        lastRotationTime = millis();

        clearAlert();
        drawEncoderValue();

        Serial.print("Encoder: ");
        Serial.println(encoderValue);
    }
} else {
    Serial.print("Encoder read failed: ");
    Serial.println(status);
}

// ========================================================
// ToF距離取得
// ========================================================
if (millis() - lastTofReadTime >= TOF_INTERVAL_MS) {
    lastTofReadTime = millis();

    status = M5Chain.getToFDistance(
        tofId,
        &tofDistance
    );

    if (status == CHAIN_OK) {
        if (tofDistance != previousTofDistance) {
            previousTofDistance = tofDistance;

            drawDistanceValue();

            Serial.print("Distance: ");
            Serial.print(tofDistance);
            Serial.println(" mm");
        }
    } else {
        Serial.print("ToF read failed: ");
        Serial.println(status);
    }
}

// ========================================================
// 5秒間回転がなければ音だけ鳴らす
// ========================================================
if (!alertActive &&
    millis() - lastRotationTime >= STOP_TIMEOUT_MS) {
    startAlert();
}

// ========================================================
// ノブ押下でカウントをリセット
// ========================================================
chain_button_press_type_t pressType;

while (M5Chain.getEncoderButtonPressStatus(
    encoderId,
    &pressType
)) {
    if (pressType == CHAIN_BUTTON_PRESS_SINGLE) {
        status = M5Chain.resetEncoderValue(
            encoderId,
            &operationStatus
        );

        if (status == CHAIN_OK && operationStatus) {
            encoderValue = 0;
            previousEncoderValue = 0;

            drawEncoderValue();

            Serial.println("Encoder reset");
        }
    }
}

delay(20);
}
