#pragma once

/**
 * @file field_extractor.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2024-11-28
 *
 * @copyright Copyright (c) 2023
 *
 */

/**
 * @brief Helper class to extract a bit field from a byte array
 *
 * Nearly all algorithms are taken from
 * https://github.com/pasztorpisti/hid-report-parser/tree/master/src
 *
 * Extracting bits from a bitstream is something already invented :-)
 */
class FieldExtractor {
  private:
    /// Position of the field in bits
    size_t offset_;
    /// Length of the field in bits
    size_t length_;

    /// Is set to true if offset and length are dividable by 8
    /// Speeds up extraction process
    bool byte_aligned_;

    /// True for Relative data, False for Absolute
    bool signed_;

  public:
    /**
     * @brief Prepares this bit field extrator for operation
     *
     * @param offset   Position of the field in bits
     * @param length   Length of the field in bits
     * @param relative True for Relative data, False for Absolute
     */
    void configure(size_t offset, size_t length, bool relative) {
        offset_ = offset;
        length_ = length;
        signed_ = relative;

        byte_aligned_ = (((offset | length) & 0b0111) == 0);
    }

    /**
     * @brief Extracts a field from the report
     *
     * Extracted data will always be extended to 32 bit
     *
     * @param report    Pointer to report buffer
     * @param len       Size in bytes
     * @return int32_t  Value of field
     */
    int32_t extract(const uint8_t *report, size_t len) {
        std::ignore = len;

        if (byte_aligned_) {
            // integer fields are often byte-aligned in HID descriptors
            size_t offset = offset_ >> 3;
            size_t size = length_ >> 3;
            if (signed_) {
                size_t k = offset;

                int32_t v;
                switch (size) {
                case 1:
                    v = (int8_t)report[k];
                    break;
                case 2:
                    v = (int16_t)((int16_t)report[k] | ((int16_t)report[k + 1] << 8));
                    break;
                case 3:
                    v = (int32_t)((int16_t)report[k] | ((int16_t)report[k + 1] << 8) |
                                  ((int32_t)(int8_t)report[k + 2] << 16));
                    break;
                default:
                    v = (int32_t)((int32_t)report[k] | ((int32_t)report[k + 1] << 8) | ((int32_t)report[k + 2] << 16) |
                                  ((int32_t)report[k + 3] << 24));
                    break;
                }
                return v;

            } else { // !m.signed_
                size_t k = offset;

                uint32_t v;
                switch (size) {
                case 1:
                    v = report[k];
                    break;
                case 2:
                    v = report[k] | ((uint16_t)report[k + 1] << 8);
                    break;
                case 3:
                    v = report[k] | ((uint16_t)report[k + 1] << 8) | ((uint32_t)report[k + 2] << 16);
                    break;
                default:
                    v = report[k] | ((uint16_t)report[k + 1] << 8) | ((uint32_t)report[k + 2] << 16) |
                        ((uint32_t)report[k + 3] << 24);
                    break;
                }
                return v;
            }
        } else { // !m.byte_aligned
            size_t k = offset_;

            size_t limited_size = std::min(length_, size_t(32));

            // https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
            int32_t mask = signed_ ? (uint32_t)1 << (limited_size - 1) : 0;

            uint8_t shift = (uint8_t)(k & 7);
            size_t idx = k >> 3;

            // n is the number of bytes in the report array that contain our report_size-bits long integer
            // Example: A 10-bit integer may span 2 or 3 bytes. It spans 3 bytes only if the first bit
            //          starts at the most significant bit of the first byte (in which case shift==7).
            // An unaligned 32 bit integer spans 5 bytes but the HID specification clearly states that
            // the maximum span is 4 bytes so 32 bit values can satisfy that only by being byte-aligned.
            uint8_t n = (uint8_t)((shift + limited_size + 7) >> 3);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
            uint32_t v = 0;
            switch (n) {
            case 5:
                v |= (uint32_t)report[idx + 4] << (32 - shift);
            case 4:
                v |= (uint32_t)report[idx + 3] << (24 - shift);
            case 3:
                v |= (uint32_t)report[idx + 2] << (16 - shift);
            case 2:
                v |= (uint16_t)report[idx + 1] << (8 - shift);
            case 1:
                v |= report[idx] >> shift;
            }
#pragma GCC diagnostic pop

            // zero'ing the bits above position 'limited_size'
            v &= ((uint32_t)1 << limited_size) - 1;

            if (signed_) {
                // sign-extending the limited_size-bits wide integer
                int32_t sv = (int32_t)((v ^ mask) - mask);
                return sv;
            } else {
                return v;
            }
        }

        return 0;
    }
};
