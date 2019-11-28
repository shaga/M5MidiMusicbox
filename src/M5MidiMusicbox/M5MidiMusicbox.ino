#include <M5Stack.h>
#include "BleMidi.h"
#include "EncHandle.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "PianoCat.h"
#include "Chocho.h"
#include "TwinkleLittleStar.h"

// エンコーダピン番号
static const int PinEncA = 22;
static const int PinEncB = 21;

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

// 演奏曲配列
static const MusicInfo_t Musics[] {
  PianoCatInfo,         // 猫踏んじゃった
  ChochoInfo,           // ちょうちょ
  TwinkleLitteStarInfo, // きらきら星
};

// 演奏曲数
static const int MusicCount = 3;

static const int AutoNoteOffTicks = 100 / portTICK_RATE_MS;

volatile bool isNoteOn = false;

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
              play_len = selected->data[play_pos].length - 1;
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

// 曲の変更
static void changeMusic(int next) {
  if (next < 0 || MusicCount <= next) return;
  if (selected_music_idx == next) return;

  encoderHandle.pause();
  if (xTimerIsTimerActive(handleTimerNoteOff) != pdFALSE) xTimerStop(handleTimerNoteOff, 0); 
  const MusicInfo_t *current = &Musics[selected_music_idx];
  noteOffLast(current);
  selected_music_idx = next;
  play_pos = -1;
  play_len = 0;
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.printf(Musics[selected_music_idx].name);

  switch (next) {
    case 0:
      M5.Lcd.fillRect(40, 220, 50, 20, 0xFFFFFF);
      break;
    case 1:
      M5.Lcd.fillRect(135, 220, 50, 20, 0xFFFFFF);
      break;
    case 2:
      M5.Lcd.fillRect(230, 220, 50, 20, 0xFFFFFF);
      break;
  }
  encoderHandle.resume();
}

// BLE接続完了イベント
static void onConnectionChanged(bool is_connected) {
  if (is_connected) {
    play_pos = -1;
    play_len = 0;
    //M5.Lcd.fillScreen(TFT_BLACK);
    changeMusic(0);
    encoderHandle.resume();
  } else {
    showInitMessage();
    encoderHandle.pause();
  }
}


void setup() {
  Serial.begin(115200);

  initTimerNoteOff();
  initQueue();
  // initialize M5Stack
  M5.begin();
  M5.Lcd.setTextSize(3);
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
    changeMusic(0);
  } else if (M5.BtnB.wasPressed()) {
    changeMusic(1);
  } else if (M5.BtnC.wasPressed()) {
    changeMusic(2);
  }

  
}
