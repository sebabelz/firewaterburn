#include <cstdio>
#include <cstring>
#include <esp_log.h>
#include <driver/gptimer.h>
#include <esp_task_wdt.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "EveDisplay.h"
#include "AdcConverter.h"

void initTemperatureTimer(gptimer_alarm_cb_t alarmCallback, AdcConverter &converter);
bool IRAM_ATTR readAdcChannels(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    static_cast<AdcConverter*>(user_data)->ReadChannelVoltage(ADC_PROBE_0);
    ESP_DRAM_LOGI("ADC", "Reading Channel: 0");
    return true;
}

extern "C" [[noreturn]] void app_main(void)
{
    AdcConverter adcConverter;

    adcConverter.Init();
    adcConverter.ReadChannelVoltage(ADC_PROBE_0);

    EveDisplay tft(GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_17, GPIO_NUM_16);

    tft.InitBus(SPI2_HOST, true);
    tft.InitDisplay();

    initTemperatureTimer(readAdcChannels, adcConverter);


    uint32_t sliderValue = 50;


    tft.waitForCmdExecution();
    tft.startCommand();
    tft.tag(1);
    tft.cmdSlider(140, 200, 200, 20, 0, sliderValue, 101);
    tft.cmdTrack(140, 200, 200, 20, 1);
    tft.addCommand(DL_COLOR_RGB | 0xFFAA03);
    tft.cmdProgressBar(40, 20, 20, 140, OPT_FLAT, sliderValue, 100);
    tft.endCommand();
    tft.executeCommand();



    for (;;) {
        uint32_t slider = tft.readMem32(REG_TRACKER);
        if ((slider & 0xFF) == 1)
        {
            sliderValue = (slider >> 16) * 101 / 65536;
        }

        tft.waitForCmdExecution();
        tft.startCommand();
        tft.tag(1);
        tft.cmdSlider(140, 200, 200, 20, 0, sliderValue, 101);
        tft.cmdNumber(240, 136, 31, OPT_CENTER, sliderValue);
        tft.cmdNumber(240, 80, 31, OPT_CENTER, adcConverter.getVoltage());
        tft.cmdProgressBar(40, 20, 20, 140, OPT_FLAT, sliderValue, 100);
        tft.endCommand();
        tft.executeCommand();
//        vTaskDelay(1000);
    }
}


void initTemperatureTimer(gptimer_alarm_cb_t alarmCallback, AdcConverter &converter) {
    gptimer_handle_t timer = nullptr;
    gptimer_config_t timerConfig = {};
    timerConfig.clk_src = GPTIMER_CLK_SRC_APB;
    timerConfig.direction = GPTIMER_COUNT_UP;
    timerConfig.resolution_hz = 10000;

    ESP_ERROR_CHECK(gptimer_new_timer(&timerConfig, &timer));

    gptimer_event_callbacks_t timerCallbacks = {
            .on_alarm = alarmCallback
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &timerCallbacks, &converter));

    gptimer_alarm_config_t alarmConfig = { };
    alarmConfig.alarm_count = 600000;
    alarmConfig.reload_count = 0;
    alarmConfig.flags.auto_reload_on_alarm = true;

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarmConfig));
    ESP_ERROR_CHECK(gptimer_start(timer));
}
