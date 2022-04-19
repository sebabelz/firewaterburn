#include <esp_log.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "EveDisplay.h"

#define RED                  0xFF0000UL                                                    // Red
#define GREEN                0x00FF00UL                                                    // Green
#define BLUE                 0x0000FFUL                                                    // Blue
#define WHITE                0xFFFFFFUL                                                    // White
#define BLACK                0x000000UL                                                    // Black
#define TAG(s) ((3UL<<24)|(((s)&255UL)<<0))

uint8_t spiBuffer[512] = { 0 };
static uint16_t bufferOffset = 0;

EveDisplay::EveDisplay(gpio_num_t pd, gpio_num_t mosi, gpio_num_t miso, gpio_num_t cs, gpio_num_t clk) {
    //set PDN Pin as an output
    pd_ = pd;
    gpio_set_direction(pd_, GPIO_MODE_OUTPUT);

    //map bus pins
    buscfg_.mosi_io_num = mosi;
    buscfg_.miso_io_num = miso;
    buscfg_.sclk_io_num = clk;
    buscfg_.quadwp_io_num = -1;
    buscfg_.quadhd_io_num = -1;
    //map device pin
    devcfg_.spics_io_num = cs;

    cmdBuffer = CommandBuffer(512);
}

void EveDisplay::InitBus(spi_host_device_t hostId, bool useDMA) {
    // Initialize the SPI bus
    spiDevice_ = hostId;

    buscfg_.max_transfer_sz = useDMA ? 0 : 4096;

    esp_err_t error = spi_bus_initialize(spiDevice_, &buscfg_, useDMA ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED);
    assert(error == ESP_OK);

    devcfg_.mode = 0;
    devcfg_.clock_speed_hz = 20 * 1000 * 1000;
    devcfg_.queue_size = 1;
    devcfg_.pre_cb = nullptr;
    devcfg_.post_cb = nullptr;

    error = spi_bus_add_device(spiDevice_, &devcfg_, &spiHandle_);
    assert(error == ESP_OK);

    //TODO return error for logging
    //return error
}

void EveDisplay::InitDisplay() {
    powerUp();
    writeCmd(FT800_ACTIVE);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    writeCmd(FT800_CLKEXT);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    writeCmd(FT800_CLK48M);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    uint8_t res = readMem8(REG_ID);

    ESP_LOGI("TFT", "Read %02X from REG_ID", res);

    writeMem16(REG_HSIZE, displaySettings_.lcdWidth);
    writeMem16(REG_HCYCLE, displaySettings_.lcdHcycle);
    writeMem16(REG_HOFFSET, displaySettings_.lcdHoffset);
    writeMem16(REG_HSYNC0, displaySettings_.lcdHsync0);
    writeMem16(REG_HSYNC1, displaySettings_.lcdHsync1);

    writeMem16(REG_VSIZE, displaySettings_.lcdHeight);
    writeMem16(REG_VCYCLE, displaySettings_.lcdVcycle);
    writeMem16(REG_VOFFSET, displaySettings_.lcdVoffset);
    writeMem16(REG_VSYNC0, displaySettings_.lcdVsync0);
    writeMem16(REG_VSYNC1, displaySettings_.lcdVsync1);

    writeMem8(REG_SWIZZLE, displaySettings_.lcdSwizzle);
    writeMem8(REG_PCLK_POL, displaySettings_.lcdPclkpol);
    writeMem8(REG_CSPREAD, displaySettings_.lcdCspread);

    // Configure Touch and Audio - not used in this example, so disable both

    writeMem8(REG_TOUCH_MODE, ZERO);                                    // Disable touch
    writeMem16(REG_TOUCH_RZTHRESH, ZERO);                        // Eliminate any false touches

    writeMem8(REG_VOL_PB, ZERO);                                            // turn recorded audio volume down
    writeMem8(REG_VOL_SOUND, ZERO);                                    // turn synthesizer volume down
    writeMem16(REG_SOUND, 0x6000);                                        // set synthesizer to mute

    displayList_ = RAM_DL;
    writeMem32(displayList_, DL_CLEAR_RGB);
    displayList_ += 4;
    writeMem32(displayList_, (DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG));
    displayList_ += 4;
    writeMem32(displayList_, DL_DISPLAY);

    writeMem32(REG_DLSWAP, DLSWAP_FRAME);
    displayList_ = RAM_DL;

    uint8_t gpio = readMem8(REG_GPIO);
    gpio |= 0x80;

    writeMem8(REG_GPIO, gpio);
    writeMem8(REG_PCLK, displaySettings_.lcdPclk);


    writeMem8(REG_PWM_DUTY, 0x30);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void EveDisplay::powerUp() {
    gpio_set_level(pd_, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    gpio_set_level(pd_, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    gpio_set_level(pd_, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

void EveDisplay::enableTouch(bool enable) {
    //TODO handle touch
    writeMem8(REG_TOUCH_MODE, ZERO);                                    // Disable touch
    writeMem16(REG_TOUCH_RZTHRESH, ZERO);                        // Eliminate any false touches
}

void EveDisplay::enableAudio(bool enable) {
    //TODO handle audio
    writeMem8(REG_VOL_PB, ZERO);                                            // turn recorded audio volume down
    writeMem8(REG_VOL_SOUND, ZERO);                                    // turn synthesizer volume down
    writeMem16(REG_SOUND, 0x6000);                                        // set synthesizer to mute
}

void EveDisplay::writeCmd(uint8_t cmd) {
    spi_transaction_t t = {};
    uint8_t data[3] = {cmd, 0x00, 0x00};

    t.length = 24;
    t.tx_buffer = data;

    spi_device_transmit(spiHandle_, &t);
}

void EveDisplay::writeMem8(uint32_t address, uint8_t data) {
    spi_transaction_t t = {};
    uint8_t txBuffer[4] = {0};

    prepareAddress(address, txBuffer);
    txBuffer[0] |= MEM_WRITE;
    txBuffer[3] = data;

    t.length = 32;
    t.tx_buffer = txBuffer;

    spi_device_polling_transmit(spiHandle_, &t);
}

void EveDisplay::writeMem16(uint32_t address, uint16_t data) {
    spi_transaction_t t = {};
    uint8_t txBuffer[5] = {0};

    prepareAddress(address, txBuffer);
    txBuffer[0] |= MEM_WRITE;

    txBuffer[4] = data >> 8;
    txBuffer[3] = data;

    t.length = 40;
    t.tx_buffer = txBuffer;

    spi_device_polling_transmit(spiHandle_, &t);
}

void EveDisplay::writeMem32(uint32_t address, uint32_t data) {
    spi_transaction_t t = {};
    uint8_t txBuffer[7] = {0};

    prepareAddress(address, txBuffer);
    txBuffer[0] |= MEM_WRITE;
    txBuffer[6] = data >> 24;
    txBuffer[5] = data >> 16;
    txBuffer[4] = data >> 8;
    txBuffer[3] = data;

    t.length = 56;
    t.tx_buffer = txBuffer;

    spi_device_polling_transmit(spiHandle_, &t);
}

uint8_t EveDisplay::readMem8(uint32_t address) {
    spi_transaction_t t = {};
    uint8_t txBuffer[5] = {0};
    uint8_t rxBuffer[5] = {0};

    prepareAddress(address, txBuffer);

    t.length = 40;
    t.tx_buffer = txBuffer;
    t.rx_buffer = rxBuffer;


    spi_device_polling_transmit(spiHandle_, &t);

    return rxBuffer[4];
}

uint16_t EveDisplay::readMem16(uint32_t address) {
    spi_transaction_t t = {};

    uint8_t txBuffer[6] = {0};
    uint8_t rxBuffer[6] = {0};

    prepareAddress(address, txBuffer);

    t.length = 48;
    t.tx_buffer = txBuffer;
    t.rx_buffer = rxBuffer;


    spi_device_polling_transmit(spiHandle_, &t);

    uint16_t data = rxBuffer[5] << 8 | rxBuffer[4];

    return data;
}

uint32_t EveDisplay::readMem32(uint32_t address) {
    spi_transaction_t t = {};

    uint8_t txBuffer[8] = {0};
    uint8_t rxBuffer[8] = {0};

    prepareAddress(address, txBuffer);

    t.length = 64;
    t.tx_buffer = txBuffer;
    t.rx_buffer = rxBuffer;


    spi_device_polling_transmit(spiHandle_, &t);

    uint32_t data = rxBuffer[7] << 24 |
                    rxBuffer[6] << 16 |
                    rxBuffer[5] << 8 |
                    rxBuffer[4];

    return data;
}

void EveDisplay::prepareAddress(uint32_t address, uint8_t *txData) {
    txData[0] = (uint8_t) ((address >> 16) | MEM_READ);
    txData[1] = (uint8_t) (address >> 8);
    txData[2] = (uint8_t) address;
    txData[3] = 0x00;
}

void EveDisplay::TestDisplay() {
    do {
        readBuffer_ = readMem16(REG_CMD_READ);
        writeBuffer_ = readMem16(REG_CMD_WRITE);
    } while (readBuffer_ != writeBuffer_);

    cmdOffset_ = writeBuffer_;

    if (color_ != WHITE) {
        color_ = WHITE;
        ESP_LOGI("TFT", "white");
    } else {
        color_ = RED;
        ESP_LOGI("TFT", "red");
    }

    writeMem32(RAM_CMD + incCmdOffset(4), CMD_DLSTART);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_COLOR_RGB | 0xFFA840);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_COLOR_RGB | color_);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_POINT_SIZE(point_size_));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_BEGIN(POINTS));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_VERTEX2F(point_x_, point_y_));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_END);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_DISPLAY);
    writeMem32(RAM_CMD + incCmdOffset(4), CMD_SWAP);

    writeMem32(REG_CMD_WRITE, cmdOffset_);
}

void EveDisplay::calibrateToucht() {

}

void EveDisplay::print(int16_t x, int16_t y, int16_t font, int16_t options, char *text) {

    do {
        readBuffer_ = readMem16(REG_CMD_READ);
        writeBuffer_ = readMem16(REG_CMD_WRITE);
    } while (readBuffer_ != writeBuffer_);

    cmdOffset_ = writeBuffer_;

    if (color_ != WHITE) {
        color_ = WHITE;
        ESP_LOGI("TFT", "white");
    } else {
        color_ = RED;
        ESP_LOGI("TFT", "red");
    }

    writeMem32(RAM_CMD + incCmdOffset(4), CMD_DLSTART);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_COLOR_RGB | 0xFFA840);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);

    writeMem32(RAM_CMD + incCmdOffset(4), DL_COLOR_RGB | color_);

    writeMem32(RAM_CMD + incCmdOffset(4), CMD_TEXT);
    writeMem16(RAM_CMD + incCmdOffset(2), x);
    writeMem16(RAM_CMD + incCmdOffset(2), y);
    writeMem16(RAM_CMD + incCmdOffset(2), 31);
    writeMem16(RAM_CMD + incCmdOffset(2), OPT_CENTER);

//    writeMem8(RAM_CMD + incCmdOffset(1), 'T');
//    writeMem8(RAM_CMD + incCmdOffset(1), 'E');
//    writeMem8(RAM_CMD + incCmdOffset(1), 'S');
//    writeMem8(RAM_CMD + incCmdOffset(1), 'T');
//    writeMem8(RAM_CMD + incCmdOffset(1), '!');
//    writeMem8(RAM_CMD + incCmdOffset(1), 0x00);
//    writeMem8(RAM_CMD + incCmdOffset(1), 0x00);
//    writeMem8(RAM_CMD + incCmdOffset(1), 0x00);

    uint8_t txBuffer[11] = { 0, 0, 0,  'T', 'E', 'S', 'T', '!', 0x00, 0x00, 0x00 };

    prepareAddress(RAM_CMD + incCmdOffset(8), txBuffer);
    txBuffer[0] |= MEM_WRITE;
    txBuffer[3] = 'T';
    spi_transaction_t t = {};

    t.tx_buffer = txBuffer;
    t.length = 11 * 8;

    spi_device_polling_transmit(spiHandle_, &t);

    writeMem32(RAM_CMD + incCmdOffset(4), DL_DISPLAY);
    writeMem32(RAM_CMD + incCmdOffset(4), CMD_SWAP);

    writeMem32(REG_CMD_WRITE, cmdOffset_);
}

