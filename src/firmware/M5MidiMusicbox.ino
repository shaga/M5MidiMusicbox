#include <M5Stack.h>
#include "BleMidi.h"

#define NOTE_LEN_1      (1600)
#define NOTE_LEN_2      (800)
#define NOTE_LEN_4      (400)
#define NOTE_LEN_8      (200)
#define NOTE_LEN_16     (100)

typedef struct {
  uint8_t   note;
  uint16_t  length;
  uint8_t   buf;
} NoteValue_t;

static const int PinEncA = 22;
static const int PinEncB = 21;

static const uint16_t BpmBase = NOTE_LEN_4;
static const int MusicBpm = 84;
static const NoteValue_t Notes[] = {
  {79, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_4, 0},
  {77, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_4, 0},
  {72, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {77, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {79, NOTE_LEN_4, 0},
  {79, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {77, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {72, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_4, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {74, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {77, NOTE_LEN_4, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {77, NOTE_LEN_8, 0},
  {79, NOTE_LEN_4, 0},
  {79, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {77, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {74, NOTE_LEN_4, 0},
  {72, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {79, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_8, 0},
  {76, NOTE_LEN_4, 0},
};

static const uint32_t NotesLength = sizeof(Notes) / sizeof(NoteValue_t);

BleMidi ble_midi;

bool is_playing = false;
int prev_enc_a = LOW;
volatile int enc_counter = 0;


static void playMusic(void* parameter) {
  
}

static void showInitMessage() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("wait to connect");
}

static void updateCount() {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("COUNT:%4d", enc_counter);
}

static void onConnectionChanged(bool is_connected) {
  if (is_connected) {
    enc_counter = 0;
    M5.Lcd.fillScreen(BLACK);
    updateCount();
  } else {
    enc_counter = 0;
    showInitMessage();
  }
}


void setup() {
  Serial.begin(115200);

  // initialize encoder pins
  pinMode(PinEncA,INPUT_PULLUP);
  pinMode(PinEncB,INPUT_PULLUP);

  // initialize M5Stack
  m5.begin();
  M5.Lcd.setTextSize(3);

  showInitMessage();

  ble_midi.registerCallback(onConnectionChanged);
  ble_midi.begin();
  

  Serial.println("start");
}


void loop() {
  // put your main code here, to run repeatedly:
  if (!ble_midi.isConnected()) return;

  if (is_playing) {

  }

  int enc_a = digitalRead(PinEncA);
  if (prev_enc_a == HIGH && enc_a == LOW) {
    if (digitalRead(PinEncB) == LOW) {
      Serial.println("count up");
      enc_counter++;
    } else if (enc_counter > 0) {
      Serial.println("count down");
      enc_counter--;
    }
    updateCount();
  }
  prev_enc_a = enc_a;

  M5.update();
}
