#include <cstdio>
#include <cstring>
#include <esp_log.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "EveDisplay.h"

extern "C" [[noreturn]] void app_main(void)
{
    EveDisplay tft(GPIO_NUM_15, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_17, GPIO_NUM_16);

    tft.InitBus(SPI2_HOST, true);
    tft.InitDisplay();
    tft.print(240, 136, 31, OPT_CENTER, "Hallo Florian");
    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}