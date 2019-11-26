#ifndef __ENC_HANDLE_H__
#define __ENC_HANDLE_H__

#include "Arduino.h"
#include "driver/pcnt.h"

class EncHandle {
private:
    #define HistoryCount    (10)

public:
    static const pcnt_unit_t kPCntUnit;
    EncHandle(int enc_a_pin, int enc_b_pin);
    void begin(QueueHandle_t queue, bool to_resume = false);
    void loop();

    void resume();
    void pause();

private:
    static const pcnt_channel_t kPCntChannel;
    static const int kFilterValue;
    static const int16_t kLimitValue;
    
    int enc_a_pin_;
    int enc_b_pin_;
    bool is_running_;
};


#endif //__ENC_HANDLE_H__
