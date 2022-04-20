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

    cmdBuffer = CommandBuffer(4096);
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
    uint8_t buffer[3] = {cmd, 0x00, 0x00};

    spiTransfer(buffer, nullptr, sizeof(buffer) / sizeof(uint8_t), SPI_WRITE, SPI_SEND_POLLING);
}

void EveDisplay::writeMem8(uint32_t address, uint8_t data) {

    uint8_t buffer[4] = { 0 };

    addAddressToBuffer(address | MEM_WRITE, buffer);

    buffer[3]= data;

    spiTransfer(buffer, nullptr, sizeof(buffer) / sizeof(uint8_t), SPI_WRITE, SPI_SEND_POLLING);
}

void EveDisplay::writeMem16(uint32_t address, uint16_t data) {
    uint8_t buffer[5] = {0};

    addAddressToBuffer(address | MEM_WRITE, buffer);

    buffer[4] = data >> 8;
    buffer[3] = data;

    spiTransfer(buffer, nullptr, sizeof(buffer) / sizeof(uint8_t), SPI_WRITE, SPI_SEND_POLLING);
}

void EveDisplay::writeMem32(uint32_t address, uint32_t data) {
    uint8_t buffer[7] = {0};

    addAddressToBuffer(address | MEM_WRITE, buffer);

    buffer[6] = data >> 24;
    buffer[5] = data >> 16;
    buffer[4] = data >> 8;
    buffer[3] = data;

    spiTransfer(buffer, nullptr, sizeof(buffer) / sizeof(uint8_t), SPI_WRITE, SPI_SEND_POLLING);
}

uint8_t EveDisplay::readMem8(uint32_t address) {
    uint8_t txBuffer[5] = {0};
    uint8_t rxBuffer[5] = {0};

    addAddressToBuffer(address | MEM_READ, txBuffer);

    spiTransfer(txBuffer, rxBuffer, sizeof(txBuffer) / sizeof(uint8_t), SPI_READ, SPI_SEND_POLLING);

    return rxBuffer[4];
}

uint16_t EveDisplay::readMem16(uint32_t address) {
    uint8_t txBuffer[6] = {0};
    uint8_t rxBuffer[6] = {0};

    addAddressToBuffer(address | MEM_READ, txBuffer);

    spiTransfer(txBuffer, rxBuffer, sizeof(txBuffer) / sizeof(uint8_t), SPI_READ, SPI_SEND_POLLING);

    uint16_t data = rxBuffer[5] << 8 | rxBuffer[4];

    return data;
}

uint32_t EveDisplay::readMem32(uint32_t address) {
    uint8_t txBuffer[8] = {0};
    uint8_t rxBuffer[8] = {0};

    addAddressToBuffer(address | MEM_READ, txBuffer);

    spiTransfer(txBuffer, rxBuffer, sizeof(txBuffer) / sizeof(uint8_t), SPI_READ, SPI_SEND_POLLING);

    uint32_t data = rxBuffer[7] << 24 |
                    rxBuffer[6] << 16 |
                    rxBuffer[5] << 8 |
                    rxBuffer[4];

    return data;
}

void EveDisplay::addAddressToBuffer(uint32_t address, uint8_t *buffer) {
    buffer[0] = (uint8_t) (address >> 16);
    buffer[1] = (uint8_t) (address >> 8);
    buffer[2] = (uint8_t) address;
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
    writeMem32(RAM_CMD + incCmdOffset(4), DL_POINT_SIZE(800));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_BEGIN(POINTS));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_VERTEX2F(240 * 16, 136 * 16));
    writeMem32(RAM_CMD + incCmdOffset(4), DL_END);
    writeMem32(RAM_CMD + incCmdOffset(4), DL_DISPLAY);
    writeMem32(RAM_CMD + incCmdOffset(4), CMD_SWAP);

    writeMem32(REG_CMD_WRITE, cmdOffset_);
}

void EveDisplay::calibrateTouch() {

}

void EveDisplay::print(int16_t x, int16_t y, int16_t font, int16_t options, std::string str) {
    do {
        readBuffer_ = readMem16(REG_CMD_READ);
        writeBuffer_ = readMem16(REG_CMD_WRITE);
    } while (readBuffer_ != writeBuffer_);

    cmdBuffer.setCmdOffset(writeBuffer_);

    cmdBuffer.addCommand((RAM_CMD + cmdBuffer.getCmdOffset()) | MEM_WRITE, CMD_DLSTART);
    cmdBuffer.addCommand(DL_COLOR_RGB | 0xFFA840);
    cmdBuffer.addCommand(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    cmdBuffer.addCommand(DL_COLOR_RGB | WHITE);
    cmdBuffer.addCommand(CMD_TEXT);
    cmdBuffer.addCommand(x);
    cmdBuffer.addCommand(y);
    cmdBuffer.addCommand(font);
    cmdBuffer.addCommand(options);

    for(auto c: str)
    {
        cmdBuffer.addCommand(c);
    }

    auto toAlign = 4 - (str.length() % 4);

    for (int i = 0; i < toAlign; ++i) {
        cmdBuffer.addCommand('\0');
    }

    cmdBuffer.addCommand(DL_DISPLAY);
    cmdBuffer.addCommand(CMD_SWAP);

    ESP_LOGE(TAG, "%d", cmdBuffer.getCmdOffset());

    spiTransfer(cmdBuffer(), nullptr, cmdBuffer.length(), SPI_WRITE, SPI_SEND_POLLING);

    writeMem32(REG_CMD_WRITE, cmdBuffer.getCmdOffset());
    cmdBuffer.clearBuffer();
}

uint16_t EveDisplay::incCmdOffset(uint8_t cmdSize) {
    uint16_t oldOffset = cmdOffset_;

    cmdOffset_ += cmdSize;
    cmdOffset_ &= 0x0FFF; // &= 4095

    return oldOffset;
}

void EveDisplay::spiTransfer(uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t length, spi_send_mode_t mode, spi_send_type_t type)
{
    assert(length != 0);
    assert(mode != (SPI_WRITE | SPI_READ));

    spi_transaction_t t = {};

    if (length <= 4 && txBuffer != nullptr)
    {
        t.flags = SPI_TRANS_USE_TXDATA;
        memcpy(t.tx_data, txBuffer, length);
    }
    else
    {
        t.tx_buffer = txBuffer;
    }

    t.length = 8 * length;

    if(mode == SPI_READ)
    {
        t.rx_buffer = rxBuffer;
    }

    switch (type) {
        case SPI_SEND_POLLING:
            spi_device_polling_transmit(spiHandle_, &t);
            break;
        case SPI_SEND_SYNCHRONOUS:
            spi_device_transmit(spiHandle_, &t);
            break;
        case SPI_SEND_QUEUED:
            break;
        default:
            ESP_LOGE(TAG, "No SPI transaction type selected");
    }
}
