#pragma once

#include "hardware/flash.h"

// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
#define FLASH_TARGET_OFFSET (256 * 1024)

const uint8_t *flash_target_contents =
    (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

#ifdef DEBUG
static inline void print_buf(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        PRINTF("%02x", buf[i]);
        if (i % 16 == 15)
            PRINTF("\n");
        else
            PRINTF(" ");
    }
}
#endif

class SingleByteFlashEepromEmulation {
  private:
    uint32_t next_write_offset_{0};
    uint8_t config_byte_cache_{0};

    std::array<uint8_t, FLASH_PAGE_SIZE> page_buffer_;

  public:
    uint8_t get_config_byte() { return config_byte_cache_; }

    SingleByteFlashEepromEmulation() {
        next_write_offset_ = 0;

        while (next_write_offset_ < FLASH_SECTOR_SIZE) {
            if (flash_target_contents[next_write_offset_] == 0xff) {
                // First clean byte found

                if (next_write_offset_ > 0) {
                    config_byte_cache_ =
                        flash_target_contents[next_write_offset_ - 1];

                    PRINTF("Found configuration byte 0x%x at offset 0x%lx\n",
                           config_byte_cache_, next_write_offset_ - 1);
                    return;
                } else {
                    PRINTF("No configuration byte found. Write at 0x%lx\n",
                           next_write_offset_);
                    config_byte_cache_ = 0;
                    return;
                }
            }
            next_write_offset_++;
        }

        // this should never happen!
    }

    /**
     * @brief Writes configuration to flash
     * This function takes 374 us to execute.
     * @param config    Configuration byte to store
     */
    void write_config(uint8_t config) {

        // The top bit must never be 1, to avoid 0xff which is detected
        // as an unwritten byte
        config &= 0x7f;

        if (config_byte_cache_ == config) {
            PRINTF("Value was already written\n");
            return;
        }

        if (next_write_offset_ >= FLASH_SECTOR_SIZE) {
            PRINTF("Erase sector at 0x%x\n", FLASH_TARGET_OFFSET);
            flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
            next_write_offset_ = 0;
        }

        uint32_t page_offset = next_write_offset_ & ~0xff;
        uint32_t in_page_offset = next_write_offset_ & 0xff;
        uint32_t page = next_write_offset_ >> 8;
        std::ignore = page; // in case PRINTF is removed via preprocessor

        PRINTF("Reading page %lu from 0x%p\n", page,
               &flash_target_contents[page_offset]);
        memcpy(page_buffer_.data(), &flash_target_contents[page_offset],
               page_buffer_.size());

        PRINTF("Setting 0x%x at offset 0x%lx\n", config, in_page_offset);

        page_buffer_[in_page_offset] = config;
        // print_buf(page_buffer_.data(), page_buffer_.size());

        PRINTF("Program page %lu at 0x%lx\n", page,
               FLASH_TARGET_OFFSET + page_offset);

        flash_range_program(FLASH_TARGET_OFFSET + page_offset,
                            page_buffer_.data(), page_buffer_.size());

        config_byte_cache_ = config;
        next_write_offset_++;
    }
};
