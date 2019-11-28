#ifndef __BLE_MIDI_H__
#define __BLE_MIDI_H__

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "MusicData.h"

#define MIDI_SERVICE_UUID           "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID    "7772e5db-3868-4112-a1a9-f2669d106bf3"
#define DEFAULT_DEVICE_NAME         "M5MusicBox"
#define IDX_BUFFER_CHANNEL          (2)
#define IDX_BUFFER_CONTROL          (3)
#define IDX_BUFFER_NOTE             (3)
#define IDX_BUFFER_CONTROL_DATA     (4)
#define IDX_BUFFER_VELOCITY         (4)
#define IDX_BUFFER_CHANNEL_2        (6)
#define IDX_BUFFER_NOTE_2           (7)
#define IDX_BUFFER_VELOCITY_2       (8)
#define MIDI_BUFFER_LENGTH          (9)
#define MIDI_NOTE_SIZE              (4)
#define MIDI_HEADER_SIZE            (1)
#define DEFAULT_NOTE_ON_VELOCITY    (120)
#define CONTROL_NO_EXPRESSION       (11)
#define CONTRLO_CHANGE_LENGTH       (5)
#define NOTE_ON_1_NOTE              (5)
#define FADEOUT_SIZE                (12)

typedef void (*ConnectionCallback_t)(bool);


class BleMidi {
public:
    BleMidi();
    void registerCallback(ConnectionCallback_t callback);
    void begin();
    void begin(std::string device_name);
    bool isConnected();
    void noteOn(uint8_t channel, uint8_t note);
    void noteOff(uint8_t channel, uint8_t note);

    void noteOn(uint8_t channel, const NoteData_t* note);
    void noteOff(uint8_t channel, const NoteData_t* note);
    bool fadeOut(uint8_t channel);
    
private:
    uint8_t maskChannelNoteOn(uint8_t channel) { return (channel & 0x0f) | 0x90; }
    uint8_t maskChannelContorlChange(uint8_t channel) { return (channel & 0x0F) | 0xB0; }
    uint8_t maskNote(uint8_t note) { return note & 0x7f; }
    uint8_t maskVelocity(uint8_t velocity) { return velocity & 0x7f; }
    void notify();
    void notify(int size);

    BLECharacteristic *characteristic_;
    uint8_t buffer_[MIDI_BUFFER_LENGTH] = {0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00};
    uint8_t velocity_;
};

#endif //__BLE_MIDI_H__
