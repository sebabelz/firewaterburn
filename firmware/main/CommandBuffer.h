#pragma once

#include <cstdint>
#include <esp_log.h>

class CommandBuffer {

private:
    const char* TAG = "CMD_BUFFER";

    uint16_t size_;
    uint16_t offset_;
    uint16_t cmdOffset_;

    uint8_t *buffer_;

    void incOffset(uint16_t &size);
    void incOffset(uint16_t &&size);

    void incCmdOffset(uint16_t &size);
    void incCmdOffset(uint16_t &&size);
public:
    CommandBuffer();
    explicit CommandBuffer(uint16_t size);
    ~CommandBuffer();

    void clearBuffer();

    uint16_t length();

    void setCmdOffset(uint16_t const &offset);
    uint16_t getCmdOffset();

    uint8_t *getBuffer();
    uint8_t* operator() ();

    template<typename T>
    void addCommand(T &&command)
    {
        ESP_LOGI(TAG, "%d", sizeof(T));
        for(uint8_t i=0; i < sizeof(T); ++i) {
            buffer_[offset_ + i] = command >> (8 * i);
        }
        incCmdOffset(sizeof(T));
    }

    template<typename T>
    void addCommand(uint32_t &&address, T &&command)
    {
        buffer_[offset_] = address >> 16;
        buffer_[offset_ + 1] = address >> 8;
        buffer_[offset_ + 2] = address;
        incOffset(3);

        addCommand(command);
    }
};
