#include "BleMidi.h"
#include "HardwareSerial.h"

static bool is_connected = false;

static ConnectionCallback_t connection_callback = NULL;

class MyBleCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *server) {
        is_connected = true;
        Serial.println("connected");
        if (connection_callback != NULL) {
            connection_callback(true);
        }
    }

    void onDisconnect(BLEServer *server) {
        is_connected = false;
        Serial.println("disconnected");
        if (connection_callback != NULL) {
            connection_callback(false);
        }
    }
};


BleMidi::BleMidi() {
    characteristic_ = NULL;
    connection_callback = NULL;
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
    noteOn(channel, note, DEFAULT_NOTE_ON_VELOCITY);
}

void BleMidi::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!is_connected) return;
    buffer_[IDX_BUFFER_CHANNEL] = maskChannel(channel);
    buffer_[IDX_BUFFER_NOTE] = maskNote(note);
    buffer_[IDX_BUFFER_VELOCITY] = maskVelocity(velocity);

    notify();
}

void BleMidi::noteOff(uint8_t channel, uint8_t note) {
    if (!is_connected) return;
    buffer_[IDX_BUFFER_CHANNEL] = maskChannel(channel);
    buffer_[IDX_BUFFER_NOTE] = maskNote(note);
    buffer_[IDX_BUFFER_VELOCITY] = 0;

    notify();
}


void BleMidi::notify() {
    if (!is_connected) return;
    characteristic_->setValue(buffer_, MIDI_BUFFER_LENGTH);
    characteristic_->notify();
}