#include "EncHandle.h"

EncHandle::EncHandle(int enc_a_pin, int enc_b_pin) {
    enc_a_pin_ = enc_a_pin;
    enc_b_pin_ = enc_b_pin;
}

void EncHandle::begin(int a_pin_state, int b_pin_state, int a_pin_mode, int b_pin_mode) {
    pinMode(enc_a_pin_, a_pin_mode);
    pinMode(enc_b_pin_, b_pin_mode);

    enc_a_pin_check_ = a_pin_state;
    enc_b_pin_check_ = b_pin_state;
    enc_a_prev_ = !a_pin_state;

    counter_ = 0;
}

void EncHandle::loop() {
    int current = digitalRead(enc_a_pin_);

    if (current != enc_a_prev_ && current == enc_a_pin_check_) {
        if (digitalRead(enc_b_pin_) == enc_b_pin_check_) {
            counter_++;
            last_tick_millis_ = millis();
        } else {
            counter_--;
        }
    }

    enc_a_prev_ = current;
}