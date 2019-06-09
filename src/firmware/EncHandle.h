#ifndef __ENC_HANDLE_H__
#define __ENC_HANDLE_H__

#include "Arduino.h"

class EncHandle {
public:
    EncHandle(int enc_a_pin, int enc_b_pin);
    void begin(int a_pin_state, int b_pin_state, int a_pin_mode = INPUT_PULLUP, int b_pin_mode = INPUT_PULLUP);
    void loop();

private:
    
    uint32_t last_tick_millis_;
    int counter_;
    int enc_a_prev_;
    int enc_a_pin_;
    int enc_b_pin_;
    int enc_a_pin_check_;
    int enc_b_pin_check_;
};


#endif //__ENC_HANDLE_H__
