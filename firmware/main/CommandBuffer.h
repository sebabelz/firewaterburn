#pragma once

#include <cstdint>

class CommandBuffer {

private:
    uint16_t size_;
    uint16_t offset_;
    uint8_t *buffer_{};

    uint16_t readBuffer_{};
    uint16_t writeBuffer_{};
    uint16_t cmdOffset_{};
public:
    CommandBuffer();
    explicit CommandBuffer(uint16_t size);
    ~CommandBuffer();
    void addCommand(uint32_t command);

    template <typename T> void addCommand(uint32_t address, T command);

    uint16_t incOffset(uint16_t size);
    void clearBuffer();
};
