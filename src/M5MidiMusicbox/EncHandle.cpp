#include "driver/pcnt.h"
#include "EncHandle.h"

const pcnt_channel_t EncHandle::kPCntChannel = PCNT_CHANNEL_0;
const pcnt_unit_t EncHandle::kPCntUnit = PCNT_UNIT_0;
const int EncHandle::kFilterValue = 100;
const int16_t EncHandle::kLimitValue = 2;

static QueueHandle_t queueHandle = NULL;

static void IRAM_ATTR pcnt_intr_handler(void* arg) {
    uint32_t intr_status = PCNT.int_st.val;

    if ((intr_status & BIT(EncHandle::kPCntUnit)) == 0) {
      return;
    }
    PCNT.int_clr.val = BIT(EncHandle::kPCntUnit);
    portBASE_TYPE HPTaskAwoken = pdFALSE;
    uint32_t v = 0;
    xQueueSendFromISR(queueHandle, &v, &HPTaskAwoken);
    if (HPTaskAwoken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
}

EncHandle::EncHandle(int enc_a_pin, int enc_b_pin) {
    enc_a_pin_ = enc_a_pin;
    enc_b_pin_ = enc_b_pin;
}

void EncHandle::begin(QueueHandle_t queue, bool to_resume) {
    queueHandle = queue;
    
    pcnt_config_t pcnt_config;
    // Set PCNT input signal and control GPIOs
    pcnt_config.pulse_gpio_num = enc_a_pin_;
    pcnt_config.ctrl_gpio_num = enc_b_pin_;
    pcnt_config.channel = kPCntChannel;
    pcnt_config.unit = kPCntUnit;
    // What to do on the positive / negative edge of pulse input?
    pcnt_config.pos_mode = PCNT_COUNT_INC;   // Count up on the positive edge
    pcnt_config.neg_mode = PCNT_COUNT_DIS;   // Keep the counter value on the negative edge
    // What to do when control input is low or high?
    pcnt_config.lctrl_mode = PCNT_MODE_DISABLE; // Reverse counting direction if low
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;    // Keep the primary counter mode if high
    // Set the maximum and minimum limit values to watch
    pcnt_config.counter_h_lim = kLimitValue;
    pcnt_config.counter_l_lim = 0;
    pcnt_isr_handle_t isr_handle;
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    pcnt_set_filter_value(kPCntUnit, kFilterValue);
    pcnt_filter_enable(kPCntUnit);

    pcnt_event_enable(kPCntUnit, PCNT_EVT_H_LIM);
    pcnt_counter_pause(kPCntUnit);
    pcnt_counter_clear(kPCntUnit);
    pcnt_isr_register(pcnt_intr_handler, NULL, 0, NULL);
    pcnt_intr_enable(kPCntUnit);

    if (to_resume) {
      pcnt_counter_resume(kPCntUnit);
    }
}

void EncHandle::resume() {
  if (!is_running_) {
    pcnt_counter_resume(kPCntUnit);
    is_running_ = true;
  }
}

void EncHandle::pause() {
  if (is_running_) {
    pcnt_counter_pause(kPCntUnit);
    is_running_ = false;
  }
}

void EncHandle::loop() {
  int16_t count = 0;
  esp_err_t ret = pcnt_get_counter_value(kPCntUnit, &count);

  if (ret == ESP_OK) {
    Serial.printf("count:%d\n", count);
  } else {
    Serial.printf("count failed\n");
  }
 }
