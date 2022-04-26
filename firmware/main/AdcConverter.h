//
// Created by BelzS on 27.03.2022.
//

#ifndef HELLO_WORLD_ADCPROBE_H
#define HELLO_WORLD_ADCPROBE_H

extern "C" {
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

typedef enum {
    ADC_PROBE_0 = ADC1_CHANNEL_3,
    ADC_PROBE_1 = ADC1_CHANNEL_4,
    ADC_PROBE_2 = ADC1_CHANNEL_5,
    ADC_PROBE_3 = ADC1_CHANNEL_6,
    ADC_PROBE_4 = ADC1_CHANNEL_7,
    ADC_PROBE_5 = ADC1_CHANNEL_8,
    ADC_PROBE_MAX = ADC1_CHANNEL_9
} probe_channel_t;
};

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64

class AdcConverter {
private:
    const adc_bits_width_t width = ADC_WIDTH_BIT_12;
    const adc_atten_t atten = ADC_ATTEN_DB_6;
    const adc_unit_t unit = ADC_UNIT_1;

    esp_adc_cal_characteristics_t chars;

    uint32_t rawValue = 0;
    uint32_t voltage = 0;

public:

    esp_err_t Init();
    uint32_t ReadChannelRaw(probe_channel_t channel);
    uint32_t ReadChannelVoltage(probe_channel_t channel);

    uint32_t getRawValue() const;
    uint32_t getVoltage() const;

};


#endif //HELLO_WORLD_ADCPROBE_H
