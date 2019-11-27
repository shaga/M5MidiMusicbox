#include "BleMidi.h"
#include "HardwareSerial.h"

static bool is_connected = false;

static ConnectionCallback_t connection_callback = NULL;

class MyBleCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *server) {
        is_connected = true;
        if (connection_callback != NULL) {
        Serial.println("connected");
            connection_callback(true);
        }
    }

    void onDisconnect(BLEServer *server) {
        is_connected = false;
        if (connection_callback != NULL) {
        Serial.println("disconnected");
            connection_callback(false);
        }
    }
};


BleMidi::BleMidi() {
    characteristic_ = NULL;
    connection_callback = NULL;
    velocity_ = 0;
}

void BleMidi::registerCallback(ConnectionCallback_t callback) {
    Serial.println("register callback");
    connection_callback = callback;
}

void BleMidi::begin() {
    begin(DEFAULT_DEVICE_NAME);
}

void BleMidi::begin(std::string device_name) {
    BLEDevice::init(device_name);

    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new MyBleCallbacks);
    BLEDevice::setEncryptionLevel((esp_ble_sec_act_t)ESP_LE_AUTH_REQ_SC_BOND);

    BLEService *service = server->createService(BLEUUID(MIDI_SERVICE_UUID));
    characteristic_ = service->createCharacteristic(
        BLEUUID(MIDI_CHARACTERISTIC_UUID),
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_WRITE_NR);
    
    characteristic_->addDescriptor(new BLE2902());

    service->start();

    BLESecurity *security = new BLESecurity();
    security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    security->setCapability(ESP_IO_CAP_NONE);
    security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    
    server->getAdvertising()->addServiceUUID(MIDI_SERVICE_UUID);
    server->getAdvertising()->start();
}

bool BleMidi::isConnected() {
    return is_connected;
}

void BleMidi::noteOn(uint8_t channel, uint8_t note) {
    if (!is_connected) return;

    buffer_[IDX_BUFFER_CHANNEL] = maskChannelNoteOn(channel);
    buffer_[IDX_BUFFER_NOTE] = maskNote(note);
    buffer_[IDX_BUFFER_VELOCITY] = maskVelocity(DEFAULT_NOTE_ON_VELOCITY);
    velocity_ = DEFAULT_NOTE_ON_VELOCITY;
    notify(NOTE_ON_1_NOTE);
}

void BleMidi::noteOn(uint8_t channel, const NoteData_t *note) {
  if(!is_connected) return;

  for (int i = 0; i < note->size && i < MAX_NOTE_COUNT; i++) {
    buffer_[IDX_BUFFER_CHANNEL + MIDI_NOTE_SIZE*i] = maskChannelNoteOn(channel);
    buffer_[IDX_BUFFER_NOTE + MIDI_NOTE_SIZE*i] = maskNote(note->value[i]);
    buffer_[IDX_BUFFER_VELOCITY + MIDI_NOTE_SIZE*i] = maskVelocity(DEFAULT_NOTE_ON_VELOCITY);
  }
  velocity_ = DEFAULT_NOTE_ON_VELOCITY;
  int send_size = MIDI_HEADER_SIZE + MIDI_NOTE_SIZE * note->size;
  if (send_size > MIDI_BUFFER_LENGTH) send_size = MIDI_BUFFER_LENGTH;
  notify(send_size);
}


void BleMidi::noteOff(uint8_t channel, const NoteData_t *note) {
  if(!is_connected) return;

  for (int i = 0; i < note->size && i < MAX_NOTE_COUNT; i++) {
    buffer_[IDX_BUFFER_CHANNEL + MIDI_NOTE_SIZE*i] = maskChannelNoteOn(channel);
    buffer_[IDX_BUFFER_NOTE + MIDI_NOTE_SIZE*i] = maskNote(note->value[i]);
    buffer_[IDX_BUFFER_VELOCITY + MIDI_NOTE_SIZE*i] = 0;
  }

  int send_size = MIDI_HEADER_SIZE + MIDI_NOTE_SIZE * note->size;
  if (send_size > MIDI_BUFFER_LENGTH) send_size = MIDI_BUFFER_LENGTH;
  velocity_ = 0;
  notify(send_size);
}

void BleMidi::notify(int size) {
    if (!is_connected) return;
    characteristic_->setValue(buffer_, size);
    characteristic_->notify();
}

void BleMidi::noteOff(uint8_t channel, uint8_t note) {
    if (!is_connected) return;
    buffer_[IDX_BUFFER_CHANNEL] = maskChannelNoteOn(channel);
    buffer_[IDX_BUFFER_NOTE] = maskNote(note);
    buffer_[IDX_BUFFER_VELOCITY] = 0;
    velocity_ = 0;
    notify(NOTE_ON_1_NOTE);
}


void BleMidi::notify() {
    if (!is_connected) return;
    characteristic_->setValue(buffer_, MIDI_BUFFER_LENGTH);
    characteristic_->notify();
}

bool BleMidi::fadeOut(uint8_t channel) {
  if (velocity_ == 0) return true;

  velocity_ -= 4;
  buffer_[IDX_BUFFER_CHANNEL] = maskChannelContorlChange(channel);
  buffer_[IDX_BUFFER_CONTROL] = CONTROL_NO_EXPRESSION;
  buffer_[IDX_BUFFER_CONTROL_DATA] = velocity_;

  notify(CONTRLO_CHANGE_LENGTH);

  return velocity_ == 0;
}
