#include <cstdlib>
#include <esp_heap_caps.h>
#include "CommandBuffer.h"

void CommandBuffer::incOffset(uint16_t &size) {
    offset_ += size;
}

void CommandBuffer::incOffset(uint16_t &&size) {
    offset_ += size;
}

void CommandBuffer::incCmdOffset(uint16_t &size) {
    cmdOffset_ += size;
    cmdOffset_ &= 0x0FFF; // &= 4095

    incOffset(size);
}

void CommandBuffer::incCmdOffset(uint16_t &&size) {
    cmdOffset_ += size;
    cmdOffset_ &= 0x0FFF; // &= 4095

    incOffset(size);
}

CommandBuffer::CommandBuffer() : CommandBuffer(512) {
}

CommandBuffer::CommandBuffer(uint16_t size) {
    assert(size <= 4096);
    assert(4096 % size == 0);
    offset_ = 0;
    size_ = size;

    buffer_ = (uint8_t*)heap_caps_calloc(size_, sizeof(*buffer_), MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
}

CommandBuffer::~CommandBuffer() {
    free(buffer_);
}

void CommandBuffer::clearBuffer() {
    offset_ = 0;
}

uint16_t CommandBuffer::length() {
    return offset_;
}

void CommandBuffer::setCmdOffset(const uint16_t &cmdOffset) {
    cmdOffset_ = cmdOffset;
}

uint16_t CommandBuffer::getCmdOffset() {
    return cmdOffset_;
}

uint8_t* CommandBuffer::getBuffer() {
    return buffer_;
}

uint8_t* CommandBuffer::operator()() {
    return buffer_;
}






