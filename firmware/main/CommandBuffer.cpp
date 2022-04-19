//

#include <cstdlib>
#include <esp_heap_caps.h>
#include "CommandBuffer.h"

CommandBuffer::CommandBuffer() : CommandBuffer(size_ = 512) {
}

CommandBuffer::CommandBuffer(uint16_t size) {
    assert(size < 4096);
    offset_ = 0;
    size_ = size;
    buffer_ = (uint8_t*)heap_caps_calloc(size_, sizeof(*buffer_), MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
    for(uint16_t i=0; i < size_; ++i) {
        buffer_[i] = 0;
    }

    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
}

CommandBuffer::~CommandBuffer() {
    free(buffer_);

    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
}

void CommandBuffer::clearBuffer() {
    for(uint16_t i=0; i < offset_; ++i){
        buffer_[i] = 0;
    }
}

void CommandBuffer::addCommand(uint32_t command) {

}

template<typename T>
void CommandBuffer::addCommand(uint32_t address, T command) {
    for(uint8_t i=2; i >= 0; --i) {
        buffer_[offset_] = static_cast<uint8_t>(address >> (i * 8));
        incOffset(1);
    }

    for(uint8_t i=0; i < sizeof(T); ++i) {
        buffer_[offset_ + i] = command >> ((sizeof(T) - i) * 8);
    }
    incOffset(sizeof(T));
}

uint16_t CommandBuffer::incOffset(uint16_t size) {
    uint16_t oldOffset = cmdOffset_;

    cmdOffset_ += size;
    cmdOffset_ &= 0x0FFF; // &= 4095

    return oldOffset;
}
