//
// Created by BelzS on 27.03.2022.
//


#include "AdcConverter.h"

esp_err_t AdcConverter::Init() {

    for(uint8_t ch = ADC_PROBE_0; ch < ADC_PROBE_MAX; ++ch) {
        adc1_config_width(width);
        adc1_config_channel_atten((adc1_channel_t)ch, atten);
        ESP_LOGI("ADC", "Channel: %d initialized", ch);
    }

    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, &chars);

    return ESP_OK;
}

uint32_t AdcConverter::ReadChannelRaw(probe_channel_t channel) {
    rawValue = 0;

    for(uint16_t samples = 0; samples < NO_OF_SAMPLES; ++samples) {
        rawValue += adc1_get_raw(static_cast<adc1_channel_t>(channel));
    }

    rawValue /= NO_OF_SAMPLES;
//    ESP_LOGI("ADC", "Raw: %d at ADC Channel: %d", reading, channel);

    return rawValue;
}

uint32_t AdcConverter::ReadChannelVoltage(probe_channel_t channel) {
    voltage = esp_adc_cal_raw_to_voltage(ReadChannelRaw(channel), &chars);
//    ESP_LOGI("ADC", "Voltage: %d at ADC Channel: %d", voltage, channel);
    return voltage;
}

uint32_t AdcConverter::getRawValue() const {
    return rawValue;
}

uint32_t AdcConverter::getVoltage() const {
    return voltage;
}

