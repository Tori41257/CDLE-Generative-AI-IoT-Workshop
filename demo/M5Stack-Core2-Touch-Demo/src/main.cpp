#include <Arduino.h>
#include <M5Unified.h>

namespace {

enum class DemoState : uint8_t {
  Running = 0,
  Caution,
  Stop,
};

constexpr uint8_t SPEAKER_VOLUME = 30;
constexpr uint16_t STOP_BEEP_FREQUENCY_HZ = 2000;
constexpr uint32_t STOP_BEEP_DURATION_MS = 100;

DemoState currentState = DemoState::Running;

uint16_t backgroundColor(DemoState state) {
  switch (state) {
    case DemoState::Running:
      return M5.Display.color565(0, 170, 70);
    case DemoState::Caution:
      return M5.Display.color565(255, 210, 0);
    case DemoState::Stop:
      return M5.Display.color565(255, 0, 0);
  }
  return TFT_BLACK;
}

uint16_t textColor(DemoState state) {
  return state == DemoState::Caution ? TFT_BLACK : TFT_WHITE;
}

const char* mainText(DemoState state) {
  switch (state) {
    case DemoState::Running:
      return "RUNNING";
    case DemoState::Caution:
      return "CAUTION";
    case DemoState::Stop:
      return "STOP";
  }
  return "";
}

const char* subText(DemoState state) {
  switch (state) {
    case DemoState::Running:
      return "OPERATION";
    case DemoState::Caution:
      return "ATTENTION";
    case DemoState::Stop:
      return "STOPPED";
  }
  return "";
}

void drawCentered(const char* text, int32_t y, float textSize,
                  uint16_t foreground, uint16_t background) {
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(textSize);
  M5.Display.setTextColor(foreground, background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(text, M5.Display.width() / 2, y);
}

void drawState(DemoState state) {
  const uint16_t background = backgroundColor(state);
  const uint16_t foreground = textColor(state);
  M5.Display.fillScreen(background);
  drawCentered(mainText(state), 102, 5, foreground, background);
  drawCentered(subText(state), 151, 2, foreground, background);
}

void advanceState() {
  switch (currentState) {
    case DemoState::Running:
      currentState = DemoState::Caution;
      break;
    case DemoState::Caution:
      currentState = DemoState::Stop;
      break;
    case DemoState::Stop:
      currentState = DemoState::Running;
      break;
  }

  drawState(currentState);
  if (currentState == DemoState::Stop) {
    M5.Speaker.tone(STOP_BEEP_FREQUENCY_HZ, STOP_BEEP_DURATION_MS);
  }
}

}  // namespace

void setup() {
  auto config = M5.config();
  config.internal_spk = true;
  M5.begin(config);

  M5.Display.setRotation(1);
  M5.Display.setBrightness(180);
  M5.Speaker.setVolume(SPEAKER_VOLUME);
  drawState(currentState);
}

void loop() {
  M5.update();
  const auto touch = M5.Touch.getDetail();

  // Core2の画面下にあるタッチボタン領域は状態変更に使わない。
  if (touch.wasPressed() && touch.y < M5.Display.height()) {
    advanceState();
  }

  delay(10);
}
