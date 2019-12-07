#include <M5Stack.h>
#include "BleMidi.h"
#include "EncHandle.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "driver/adc.h"
#include "PianoCat.h"
#include "Chocho.h"
#include "TwinkleLittleStar.h"

// エンコーダピン番号
static const int PinEncA = 13;
static const int PinEncB = 5;

enum ESpeed {
  kSpdFast,
  kSpdSlow,
  kSpdNormal2Fast,
  kSpdNormal2Slow,  
};

// 演奏曲配列
static const MusicInfo_t Musics[] {
  PianoCatInfo,         // 猫踏んじゃった
  ChochoInfo,           // ちょうちょ
  TwinkleLitteStarInfo, // きらきら星
};

// 演奏曲数
static const int MusicCount = 3;

static const int AutoNoteOffTicks = 50 / portTICK_RATE_MS;

static const char kStrSpdNormal[] = "Normal";
static const char kStrSpdFast[] = "Fast";
static const char kStrSpdSlow[] = "Slow";

static const char kStrEnable[] = "Enable";
static const char kStrDisable[] = "Disable";

// エンコーダ利用クラス
EncHandle encoderHandle(PinEncA, PinEncB);

// エンコーダイベントキュー
static QueueHandle_t handleEventQueue;
static QueueHandle_t handleQueueFadeOut;
static TimerHandle_t handleTimerNoteOff;
static SemaphoreHandle_t handleSemNoteOff;

// BLE Midiクラス
BleMidi ble_midi;

// 演奏位置
volatile int play_pos = 0;

// 演奏長さ
volatile int play_len = 0;

// 演奏曲
volatile int selected_music_idx = -1;

volatile bool isNoteOn = false;

volatile int speed = kSpdNormal2Fast;

bool enableVibra = false;

static inline const char* getStrSpd() {
  if (speed == kSpdFast) return kStrSpdFast;
  if (speed == kSpdSlow) return kStrSpdSlow;
  return kStrSpdNormal;
}

static inline int getSpeed() {
  if (speed == kSpdFast) return 1;
  if (speed == kSpdSlow) return 4;
  return 2;
}

static inline const char* getStrVibra() {
  return enableVibra ? kStrEnable : kStrDisable;
}

// 最終ノートオフ
static void noteOffLast(const MusicInfo_t *music) {
  if (!isNoteOn) return;
  if (play_pos >= 0) {
    if (play_pos == music->length) {
      ble_midi.noteOff(1, &music->data[music->length-1]);
    } else if (play_pos < music->length)  {
      ble_midi.noteOff(1, &music->data[play_pos]);
    }
  }
  isNoteOn = false;
}

static void taskFadeOut(void *params) {
  uint32_t v;
  while(1) {
    portBASE_TYPE res = xQueueReceive(handleQueueFadeOut, &v, 10 / portTICK_PERIOD_MS);
    if (res != pdTRUE) continue;
    if (xSemaphoreTake(handleSemNoteOff, 10 / portTICK_RATE_MS) == pdTRUE) {
      if (isNoteOn && ble_midi.fadeOut(1)) {
        const MusicInfo_t* current = &Musics[selected_music_idx];
        noteOffLast(current);
        xTimerStop(handleTimerNoteOff, 0);
      }
      xSemaphoreGive(handleSemNoteOff);
    }
  }
}

static void timerNoteOff(TimerHandle_t handle) {
    uint32_t v = 0;
    xQueueSend(handleQueueFadeOut, &v, 10 / portTICK_RATE_MS);
//
//  if (xSemaphoreTake(handleSemNoteOff, 10 / portTICK_RATE_MS) == pdTRUE) {
//    if (ble_midi.fadeOut(1)) {
//      const MusicInfo_t* current = &Musics[selected_music_idx];
//      noteOffLast(current);
//      xTimerStop(handleTimerNoteOff, 0);
//    }
//    xSemaphoreGive(handleSemNoteOff);
//  }
}

// イベントキュー初期化
static void initQueue() {
  handleEventQueue = xQueueCreate(10, sizeof(uint32_t));
  handleQueueFadeOut = xQueueCreate(10, sizeof(uint32_t));
}

static void initTimerNoteOff() {
  handleSemNoteOff = xSemaphoreCreateBinary();
  xSemaphoreGive(handleSemNoteOff);
  handleTimerNoteOff = xTimerCreate("note_off_timer", AutoNoteOffTicks, pdTRUE, NULL, timerNoteOff);
}

// BLE接続待ちメッセージ表示
static void showInitMessage() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.printf("wait to connect");
}


// 演奏スレッド処理
static void playMusic(void* parameter) {
  portBASE_TYPE res;
  uint32_t v;
  while (1) {
    // ハンドルイベントキュー受信待ち
    res = xQueueReceive(handleEventQueue, &v, 100 / portTICK_PERIOD_MS);
    if (res == pdTRUE) {
      const MusicInfo_t* selected = &Musics[selected_music_idx];
      if (play_len > 0) {
        play_len--;
      } else {
        // 前の音のノートオフ
        if (xSemaphoreTake(handleSemNoteOff, 1 / portTICK_RATE_MS) == pdTRUE) {
          if (xTimerIsTimerActive(handleTimerNoteOff) != pdFALSE) xTimerStop(handleTimerNoteOff, 0);
          noteOffLast(selected);
          xSemaphoreGive(handleSemNoteOff);
        }
        play_pos++;
        // 次の音のノートオン
        if (play_pos < selected->length) {
            if (xSemaphoreTake(handleSemNoteOff, 100 / portTICK_RATE_MS) == pdTRUE) {
              ble_midi.noteOn(1, &selected->data[play_pos]);
              play_len = (selected->data[play_pos].length - 1) * getSpeed();
              isNoteOn = true;
              xSemaphoreGive(handleSemNoteOff);
              xTimerStart(handleTimerNoteOff, 0);
            }
        } else if (play_pos == selected->length) {
          play_len = 4;
        } else {
          play_pos = -1;
        }
      }
    }
  }
}

static void displayState() {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.printf("Speed: %s\n", getStrSpd());
  M5.Lcd.setCursor(5, 25);
  M5.Lcd.printf("Vibra: %s\n", getStrVibra());
  M5.Lcd.setCursor(5, 45);
  M5.Lcd.printf("Title: %s", Musics[selected_music_idx].name);

  M5.Lcd.setCursor(40, 221);
  M5.Lcd.printf("SPEED");
  M5.Lcd.setCursor(130, 221);
  M5.Lcd.printf("VIBRA");
  M5.Lcd.setCursor(225, 221);
  M5.Lcd.printf("TITLE");
}

// 曲の変更
static void changeMusic() {

  encoderHandle.pause();
  if (xTimerIsTimerActive(handleTimerNoteOff) != pdFALSE) xTimerStop(handleTimerNoteOff, 0); 
  if (0 <= selected_music_idx && selected_music_idx < MusicCount) {
    const MusicInfo_t *current = &Musics[selected_music_idx];
    noteOffLast(current);
  }
  selected_music_idx = (selected_music_idx + 1) % MusicCount;
  play_pos = -1;
  play_len = 0;
  M5.Lcd.fillScreen(TFT_BLACK);
  displayState();
//  M5.Lcd.setCursor(5, 5);
//  M5.Lcd.printf(Musics[selected_music_idx].name);
//
//  switch (next) {
//    case 0:
//      M5.Lcd.fillRect(40, 220, 50, 20, 0xFFFFFF);
//      break;
//    case 1:
//      M5.Lcd.fillRect(135, 220, 50, 20, 0xFFFFFF);
//      break;
//    case 2:
//      M5.Lcd.fillRect(230, 220, 50, 20, 0xFFFFFF);
//      break;
//  }
  encoderHandle.resume();
}

static void changeSpeed() {
  switch (speed) {
    case kSpdNormal2Fast:
      speed = kSpdFast;
      break;
    case kSpdNormal2Slow:
      speed = kSpdSlow;
      break;
    case kSpdFast:
      speed = kSpdNormal2Slow;
      break;
    case kSpdSlow:
      speed = kSpdNormal2Fast;
      break;
    default:
      break;
  }

  displayState();
}

static void changeVibra() {
  enableVibra = ble_midi.changeVibra();

  displayState();
}

// BLE接続完了イベント
static void onConnectionChanged(bool is_connected) {
  if (is_connected) {
    play_pos = -1;
    play_len = 0;
    speed = kSpdNormal2Fast;
    enableVibra = false;
    selected_music_idx = 0;
    enableVibra = ble_midi.isEnableVibra();
    displayState();
    encoderHandle.resume();
  } else {
    showInitMessage();
    encoderHandle.pause();
  }
}

void initBatteryMonitor() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_6)
}

void setup() {
  Serial.begin(115200);

  initTimerNoteOff();
  initQueue();
  // initialize M5Stack
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextWrap(true);
  showInitMessage();
  
  ble_midi.registerCallback(onConnectionChanged);
  ble_midi.begin();

  xTaskCreate(playMusic, "playMusic", 4096, NULL, 1, NULL);
  xTaskCreate(taskFadeOut, "fadeOut", 4096, NULL, 2, NULL);
  encoderHandle.begin(handleEventQueue);
  Serial.println("start");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!ble_midi.isConnected()) return;
  M5.update();
  if (M5.BtnA.wasPressed()) {
    changeSpeed();
  } else if (M5.BtnB.wasPressed()) {
    changeVibra();
  } else if (M5.BtnC.wasPressed()) {
    changeMusic();
  }

  
}
