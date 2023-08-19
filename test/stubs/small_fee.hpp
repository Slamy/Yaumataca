/**
 * @file small_fee.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

/// Address in flash to write data to
#define FLASH_TARGET_OFFSET (256 * 1024)

/**
 * @brief Implements flash EEPROM emulation for single byte storage
 *
 */
class SingleByteFlashEepromEmulation {
  private:
    /// @brief Cache of the recovered configuration. Avoids repeated reads
    uint8_t config_byte_cache_{0};

  public:
    /**
     * @brief Returns the currently stored configuration
     *
     * @return uint8_t Value between 0x00 and 0x7f
     */
    uint8_t get_config_byte() { return config_byte_cache_; }

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
        config_byte_cache_ = config;
    }
};
