#pragma once
#include <hal/spi_types.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "EveCommands.h"
#include "DisplaySettings.h"
#include "CommandBuffer.h"

//typedef enum _spi_send_flag_t
//{
//    SPI_SEND_QUEUED			= 0x00000000,
//    SPI_SEND_POLLING		= 0x00000001,
//    SPI_SEND_SYNCHRONOUS	= 0x00000002,
//    SPI_RECEIVE				= 0x00000008,
//    SPI_ADDRESS			    = 0x0000000C,
//} spi_send_flag_t;
//
//typedef struct _spi_read_data
//{
//    uint8_t _dummy_byte;
//    union
//    {
//        uint8_t 	byte;
//        uint16_t	word;
//        uint32_t	dword;
//    } __attribute__((packed));
//} spi_read_data __attribute__((aligned(4)));

//void spi_transaction(const uint8_t* data, uint16_t length, spi_send_flag_t flags, spi_read_data* out, uint64_t addr)
//{
//    if (length == 0) return;           //no need to send anything
//
//    // wait for previous pending transaction results
//    spi_transaction_ext_t t = {};
//
//    t.base.length = length * 8; // transaction length is in bits
//
//    if (length <= 4 && data != NULL)
//    {
//        t.base.flags = SPI_TRANS_USE_TXDATA;
//        memcpy(t.base.tx_data, data, length);
//    }
//    else
//    {
//        t.base.tx_buffer = data;
//    }
//
//    if(flags & SPI_RECEIVE)
//    {
//        assert(out !=NULL && (flags & (SPI_SEND_POLLING | SPI_SEND_SYNCHRONOUS)));	// sanity check
//        t.base.rx_buffer = out;
//        t.base.rxlength = 0;	// default to same as tx length
//    }
//
//    if(flags & SPI_ADDRESS)
//    {
//        t.address_bits = 24;
//        t.base.addr = addr;
//        t.base.flags |= SPI_TRANS_VARIABLE_ADDR;
//    }
//
//    t.base.user = (void*)flags;		// save flags for pre/post transaction processing
//
//    // poll/complete/queue transaction
//    if(flags & SPI_SEND_POLLING)
//    {
//        spi_device_polling_transmit(spi, (spi_transaction_t*)&t);
//    }
//    else if (flags & SPI_SEND_SYNCHRONOUS)
//    {
//        spi_device_transmit(spi, (spi_transaction_t*)&t);
//    }
//}


class EveDisplay {
private:
    gpio_num_t pd_;
    spi_device_handle_t spiHandle_{};
    spi_host_device_t spiDevice_;
    spi_bus_config_t buscfg_ = {};
    spi_device_interface_config_t devcfg_ = {};
    DisplaySettings_t displaySettings_;
    uint32_t displayList_ = RAM_DL;

    CommandBuffer cmdBuffer;



    uint16_t point_size_ = 800;
    uint32_t point_x_ = (240 * 16);
    uint32_t point_y_ = (136 * 16);
    uint32_t color_{};

    void powerUp();
    void enableTouch(bool enable);
    void enableAudio(bool enable);

    void prepareAddress(uint32_t address, uint8_t *txData);

    uint16_t incCmdOffset(uint8_t cmdSize);

public:
    EveDisplay(gpio_num_t pd, gpio_num_t mosi, gpio_num_t miso, gpio_num_t cs, gpio_num_t clk);
    void InitBus(spi_host_device_t hostId, bool useDMA);
    void InitDisplay();
    void calibrateToucht();
    void TestDisplay();
    void writeCmd(uint8_t cmd);
    void writeMem8(uint32_t address, uint8_t data);
    void writeMem16(uint32_t address, uint16_t data);
    void writeMem32(uint32_t address, uint32_t data);

    void print(int16_t x, int16_t y, int16_t font, int16_t options, char *text);
    uint8_t readMem8(uint32_t address);
    uint16_t readMem16(uint32_t address);
    uint32_t readMem32(uint32_t address);
};