/*
 * Bakelite C Runtime
 * Copyright (c) Emma Powers 2021-2025
 *
 * C99 compatible header-only implementation for embedded systems.
 * Licensed under the MIT License (see end of file).
 */

#ifndef BAKELITE_H
#define BAKELITE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

/*
 * Common C99 implementations (CRC and COBS)
 */
/*
 * Bakelite CRC Functions (C99)
 * Copyright (c) Emma Powers 2021-2025
 *
 * Platform-independent CRC-8, CRC-16, and CRC-32 implementations
 * with flash storage support for embedded platforms.
 */

#ifndef BAKELITE_COMMON_CRC_H
#define BAKELITE_COMMON_CRC_H

#include <stdint.h>
#include <stddef.h>

/*
 * Flash storage attribute macros for embedded platforms.
 *
 * On most platforms, const variables are automatically placed in flash.
 * AVR requires PROGMEM and special read functions.
 * ESP32/ESP8266 use ICACHE_RODATA_ATTR but can read directly.
 */
#if defined(__AVR__)
  #include <avr/pgmspace.h>
  #define BAKELITE_FLASH PROGMEM
  #define BAKELITE_FLASH_READ_8(x)  pgm_read_byte(&(x))
  #define BAKELITE_FLASH_READ_16(x) pgm_read_word(&(x))
  #define BAKELITE_FLASH_READ_32(x) pgm_read_dword(&(x))
#elif defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  #ifdef ESP8266
    #include <pgmspace.h>
  #endif
  #define BAKELITE_FLASH ICACHE_RODATA_ATTR
  #define BAKELITE_FLASH_READ_8(x)  (x)
  #define BAKELITE_FLASH_READ_16(x) (x)
  #define BAKELITE_FLASH_READ_32(x) (x)
#elif defined(__XC8)
  /* Microchip XC8 for 8-bit PIC */
  #define BAKELITE_FLASH __rom
  #define BAKELITE_FLASH_READ_8(x)  (x)
  #define BAKELITE_FLASH_READ_16(x) (x)
  #define BAKELITE_FLASH_READ_32(x) (x)
#elif defined(__XC16) || defined(__XC32)
  /* Microchip XC16/XC32 - const is sufficient */
  #define BAKELITE_FLASH
  #define BAKELITE_FLASH_READ_8(x)  (x)
  #define BAKELITE_FLASH_READ_16(x) (x)
  #define BAKELITE_FLASH_READ_32(x) (x)
#else
  /* Default: const variables are in flash (ARM Cortex-M, etc.) */
  #define BAKELITE_FLASH
  #define BAKELITE_FLASH_READ_8(x)  (x)
  #define BAKELITE_FLASH_READ_16(x) (x)
  #define BAKELITE_FLASH_READ_32(x) (x)
#endif

/* CRC size constants */
#define BAKELITE_CRC_NOOP_SIZE 0
#define BAKELITE_CRC8_SIZE  1
#define BAKELITE_CRC16_SIZE 2
#define BAKELITE_CRC32_SIZE 4

/* CRC-8 polynomial: 0x107 */
static inline uint8_t bakelite_crc8(const uint8_t *data, size_t len, uint8_t crc) {
    static const uint8_t table[256] BAKELITE_FLASH = {
    0x00U,0x07U,0x0EU,0x09U,0x1CU,0x1BU,0x12U,0x15U,
    0x38U,0x3FU,0x36U,0x31U,0x24U,0x23U,0x2AU,0x2DU,
    0x70U,0x77U,0x7EU,0x79U,0x6CU,0x6BU,0x62U,0x65U,
    0x48U,0x4FU,0x46U,0x41U,0x54U,0x53U,0x5AU,0x5DU,
    0xE0U,0xE7U,0xEEU,0xE9U,0xFCU,0xFBU,0xF2U,0xF5U,
    0xD8U,0xDFU,0xD6U,0xD1U,0xC4U,0xC3U,0xCAU,0xCDU,
    0x90U,0x97U,0x9EU,0x99U,0x8CU,0x8BU,0x82U,0x85U,
    0xA8U,0xAFU,0xA6U,0xA1U,0xB4U,0xB3U,0xBAU,0xBDU,
    0xC7U,0xC0U,0xC9U,0xCEU,0xDBU,0xDCU,0xD5U,0xD2U,
    0xFFU,0xF8U,0xF1U,0xF6U,0xE3U,0xE4U,0xEDU,0xEAU,
    0xB7U,0xB0U,0xB9U,0xBEU,0xABU,0xACU,0xA5U,0xA2U,
    0x8FU,0x88U,0x81U,0x86U,0x93U,0x94U,0x9DU,0x9AU,
    0x27U,0x20U,0x29U,0x2EU,0x3BU,0x3CU,0x35U,0x32U,
    0x1FU,0x18U,0x11U,0x16U,0x03U,0x04U,0x0DU,0x0AU,
    0x57U,0x50U,0x59U,0x5EU,0x4BU,0x4CU,0x45U,0x42U,
    0x6FU,0x68U,0x61U,0x66U,0x73U,0x74U,0x7DU,0x7AU,
    0x89U,0x8EU,0x87U,0x80U,0x95U,0x92U,0x9BU,0x9CU,
    0xB1U,0xB6U,0xBFU,0xB8U,0xADU,0xAAU,0xA3U,0xA4U,
    0xF9U,0xFEU,0xF7U,0xF0U,0xE5U,0xE2U,0xEBU,0xECU,
    0xC1U,0xC6U,0xCFU,0xC8U,0xDDU,0xDAU,0xD3U,0xD4U,
    0x69U,0x6EU,0x67U,0x60U,0x75U,0x72U,0x7BU,0x7CU,
    0x51U,0x56U,0x5FU,0x58U,0x4DU,0x4AU,0x43U,0x44U,
    0x19U,0x1EU,0x17U,0x10U,0x05U,0x02U,0x0BU,0x0CU,
    0x21U,0x26U,0x2FU,0x28U,0x3DU,0x3AU,0x33U,0x34U,
    0x4EU,0x49U,0x40U,0x47U,0x52U,0x55U,0x5CU,0x5BU,
    0x76U,0x71U,0x78U,0x7FU,0x6AU,0x6DU,0x64U,0x63U,
    0x3EU,0x39U,0x30U,0x37U,0x22U,0x25U,0x2CU,0x2BU,
    0x06U,0x01U,0x08U,0x0FU,0x1AU,0x1DU,0x14U,0x13U,
    0xAEU,0xA9U,0xA0U,0xA7U,0xB2U,0xB5U,0xBCU,0xBBU,
    0x96U,0x91U,0x98U,0x9FU,0x8AU,0x8DU,0x84U,0x83U,
    0xDEU,0xD9U,0xD0U,0xD7U,0xC2U,0xC5U,0xCCU,0xCBU,
    0xE6U,0xE1U,0xE8U,0xEFU,0xFAU,0xFDU,0xF4U,0xF3U,
    };

    while (len > 0) {
        crc = BAKELITE_FLASH_READ_8(table[*data ^ crc]);
        data++;
        len--;
    }
    return crc;
}

/* CRC-16 polynomial: 0x18005, bit reverse algorithm */
static inline uint16_t bakelite_crc16(const uint8_t *data, size_t len, uint16_t crc) {
    static const uint16_t table[256] BAKELITE_FLASH = {
    0x0000U,0xC0C1U,0xC181U,0x0140U,0xC301U,0x03C0U,0x0280U,0xC241U,
    0xC601U,0x06C0U,0x0780U,0xC741U,0x0500U,0xC5C1U,0xC481U,0x0440U,
    0xCC01U,0x0CC0U,0x0D80U,0xCD41U,0x0F00U,0xCFC1U,0xCE81U,0x0E40U,
    0x0A00U,0xCAC1U,0xCB81U,0x0B40U,0xC901U,0x09C0U,0x0880U,0xC841U,
    0xD801U,0x18C0U,0x1980U,0xD941U,0x1B00U,0xDBC1U,0xDA81U,0x1A40U,
    0x1E00U,0xDEC1U,0xDF81U,0x1F40U,0xDD01U,0x1DC0U,0x1C80U,0xDC41U,
    0x1400U,0xD4C1U,0xD581U,0x1540U,0xD701U,0x17C0U,0x1680U,0xD641U,
    0xD201U,0x12C0U,0x1380U,0xD341U,0x1100U,0xD1C1U,0xD081U,0x1040U,
    0xF001U,0x30C0U,0x3180U,0xF141U,0x3300U,0xF3C1U,0xF281U,0x3240U,
    0x3600U,0xF6C1U,0xF781U,0x3740U,0xF501U,0x35C0U,0x3480U,0xF441U,
    0x3C00U,0xFCC1U,0xFD81U,0x3D40U,0xFF01U,0x3FC0U,0x3E80U,0xFE41U,
    0xFA01U,0x3AC0U,0x3B80U,0xFB41U,0x3900U,0xF9C1U,0xF881U,0x3840U,
    0x2800U,0xE8C1U,0xE981U,0x2940U,0xEB01U,0x2BC0U,0x2A80U,0xEA41U,
    0xEE01U,0x2EC0U,0x2F80U,0xEF41U,0x2D00U,0xEDC1U,0xEC81U,0x2C40U,
    0xE401U,0x24C0U,0x2580U,0xE541U,0x2700U,0xE7C1U,0xE681U,0x2640U,
    0x2200U,0xE2C1U,0xE381U,0x2340U,0xE101U,0x21C0U,0x2080U,0xE041U,
    0xA001U,0x60C0U,0x6180U,0xA141U,0x6300U,0xA3C1U,0xA281U,0x6240U,
    0x6600U,0xA6C1U,0xA781U,0x6740U,0xA501U,0x65C0U,0x6480U,0xA441U,
    0x6C00U,0xACC1U,0xAD81U,0x6D40U,0xAF01U,0x6FC0U,0x6E80U,0xAE41U,
    0xAA01U,0x6AC0U,0x6B80U,0xAB41U,0x6900U,0xA9C1U,0xA881U,0x6840U,
    0x7800U,0xB8C1U,0xB981U,0x7940U,0xBB01U,0x7BC0U,0x7A80U,0xBA41U,
    0xBE01U,0x7EC0U,0x7F80U,0xBF41U,0x7D00U,0xBDC1U,0xBC81U,0x7C40U,
    0xB401U,0x74C0U,0x7580U,0xB541U,0x7700U,0xB7C1U,0xB681U,0x7640U,
    0x7200U,0xB2C1U,0xB381U,0x7340U,0xB101U,0x71C0U,0x7080U,0xB041U,
    0x5000U,0x90C1U,0x9181U,0x5140U,0x9301U,0x53C0U,0x5280U,0x9241U,
    0x9601U,0x56C0U,0x5780U,0x9741U,0x5500U,0x95C1U,0x9481U,0x5440U,
    0x9C01U,0x5CC0U,0x5D80U,0x9D41U,0x5F00U,0x9FC1U,0x9E81U,0x5E40U,
    0x5A00U,0x9AC1U,0x9B81U,0x5B40U,0x9901U,0x59C0U,0x5880U,0x9841U,
    0x8801U,0x48C0U,0x4980U,0x8941U,0x4B00U,0x8BC1U,0x8A81U,0x4A40U,
    0x4E00U,0x8EC1U,0x8F81U,0x4F40U,0x8D01U,0x4DC0U,0x4C80U,0x8C41U,
    0x4400U,0x84C1U,0x8581U,0x4540U,0x8701U,0x47C0U,0x4680U,0x8641U,
    0x8201U,0x42C0U,0x4380U,0x8341U,0x4100U,0x81C1U,0x8081U,0x4040U,
    };

    while (len > 0) {
        crc = BAKELITE_FLASH_READ_16(table[*data ^ (uint8_t)crc]) ^ (crc >> 8);
        data++;
        len--;
    }
    return crc;
}

/* CRC-32 polynomial: 0x104C11DB7, bit reverse algorithm */
static inline uint32_t bakelite_crc32(const uint8_t *data, size_t len, uint32_t crc) {
    static const uint32_t table[256] BAKELITE_FLASH = {
    0x00000000U,0x77073096U,0xEE0E612CU,0x990951BAU,
    0x076DC419U,0x706AF48FU,0xE963A535U,0x9E6495A3U,
    0x0EDB8832U,0x79DCB8A4U,0xE0D5E91EU,0x97D2D988U,
    0x09B64C2BU,0x7EB17CBDU,0xE7B82D07U,0x90BF1D91U,
    0x1DB71064U,0x6AB020F2U,0xF3B97148U,0x84BE41DEU,
    0x1ADAD47DU,0x6DDDE4EBU,0xF4D4B551U,0x83D385C7U,
    0x136C9856U,0x646BA8C0U,0xFD62F97AU,0x8A65C9ECU,
    0x14015C4FU,0x63066CD9U,0xFA0F3D63U,0x8D080DF5U,
    0x3B6E20C8U,0x4C69105EU,0xD56041E4U,0xA2677172U,
    0x3C03E4D1U,0x4B04D447U,0xD20D85FDU,0xA50AB56BU,
    0x35B5A8FAU,0x42B2986CU,0xDBBBC9D6U,0xACBCF940U,
    0x32D86CE3U,0x45DF5C75U,0xDCD60DCFU,0xABD13D59U,
    0x26D930ACU,0x51DE003AU,0xC8D75180U,0xBFD06116U,
    0x21B4F4B5U,0x56B3C423U,0xCFBA9599U,0xB8BDA50FU,
    0x2802B89EU,0x5F058808U,0xC60CD9B2U,0xB10BE924U,
    0x2F6F7C87U,0x58684C11U,0xC1611DABU,0xB6662D3DU,
    0x76DC4190U,0x01DB7106U,0x98D220BCU,0xEFD5102AU,
    0x71B18589U,0x06B6B51FU,0x9FBFE4A5U,0xE8B8D433U,
    0x7807C9A2U,0x0F00F934U,0x9609A88EU,0xE10E9818U,
    0x7F6A0DBBU,0x086D3D2DU,0x91646C97U,0xE6635C01U,
    0x6B6B51F4U,0x1C6C6162U,0x856530D8U,0xF262004EU,
    0x6C0695EDU,0x1B01A57BU,0x8208F4C1U,0xF50FC457U,
    0x65B0D9C6U,0x12B7E950U,0x8BBEB8EAU,0xFCB9887CU,
    0x62DD1DDFU,0x15DA2D49U,0x8CD37CF3U,0xFBD44C65U,
    0x4DB26158U,0x3AB551CEU,0xA3BC0074U,0xD4BB30E2U,
    0x4ADFA541U,0x3DD895D7U,0xA4D1C46DU,0xD3D6F4FBU,
    0x4369E96AU,0x346ED9FCU,0xAD678846U,0xDA60B8D0U,
    0x44042D73U,0x33031DE5U,0xAA0A4C5FU,0xDD0D7CC9U,
    0x5005713CU,0x270241AAU,0xBE0B1010U,0xC90C2086U,
    0x5768B525U,0x206F85B3U,0xB966D409U,0xCE61E49FU,
    0x5EDEF90EU,0x29D9C998U,0xB0D09822U,0xC7D7A8B4U,
    0x59B33D17U,0x2EB40D81U,0xB7BD5C3BU,0xC0BA6CADU,
    0xEDB88320U,0x9ABFB3B6U,0x03B6E20CU,0x74B1D29AU,
    0xEAD54739U,0x9DD277AFU,0x04DB2615U,0x73DC1683U,
    0xE3630B12U,0x94643B84U,0x0D6D6A3EU,0x7A6A5AA8U,
    0xE40ECF0BU,0x9309FF9DU,0x0A00AE27U,0x7D079EB1U,
    0xF00F9344U,0x8708A3D2U,0x1E01F268U,0x6906C2FEU,
    0xF762575DU,0x806567CBU,0x196C3671U,0x6E6B06E7U,
    0xFED41B76U,0x89D32BE0U,0x10DA7A5AU,0x67DD4ACCU,
    0xF9B9DF6FU,0x8EBEEFF9U,0x17B7BE43U,0x60B08ED5U,
    0xD6D6A3E8U,0xA1D1937EU,0x38D8C2C4U,0x4FDFF252U,
    0xD1BB67F1U,0xA6BC5767U,0x3FB506DDU,0x48B2364BU,
    0xD80D2BDAU,0xAF0A1B4CU,0x36034AF6U,0x41047A60U,
    0xDF60EFC3U,0xA867DF55U,0x316E8EEFU,0x4669BE79U,
    0xCB61B38CU,0xBC66831AU,0x256FD2A0U,0x5268E236U,
    0xCC0C7795U,0xBB0B4703U,0x220216B9U,0x5505262FU,
    0xC5BA3BBEU,0xB2BD0B28U,0x2BB45A92U,0x5CB36A04U,
    0xC2D7FFA7U,0xB5D0CF31U,0x2CD99E8BU,0x5BDEAE1DU,
    0x9B64C2B0U,0xEC63F226U,0x756AA39CU,0x026D930AU,
    0x9C0906A9U,0xEB0E363FU,0x72076785U,0x05005713U,
    0x95BF4A82U,0xE2B87A14U,0x7BB12BAEU,0x0CB61B38U,
    0x92D28E9BU,0xE5D5BE0DU,0x7CDCEFB7U,0x0BDBDF21U,
    0x86D3D2D4U,0xF1D4E242U,0x68DDB3F8U,0x1FDA836EU,
    0x81BE16CDU,0xF6B9265BU,0x6FB077E1U,0x18B74777U,
    0x88085AE6U,0xFF0F6A70U,0x66063BCAU,0x11010B5CU,
    0x8F659EFFU,0xF862AE69U,0x616BFFD3U,0x166CCF45U,
    0xA00AE278U,0xD70DD2EEU,0x4E048354U,0x3903B3C2U,
    0xA7672661U,0xD06016F7U,0x4969474DU,0x3E6E77DBU,
    0xAED16A4AU,0xD9D65ADCU,0x40DF0B66U,0x37D83BF0U,
    0xA9BCAE53U,0xDEBB9EC5U,0x47B2CF7FU,0x30B5FFE9U,
    0xBDBDF21CU,0xCABAC28AU,0x53B39330U,0x24B4A3A6U,
    0xBAD03605U,0xCDD70693U,0x54DE5729U,0x23D967BFU,
    0xB3667A2EU,0xC4614AB8U,0x5D681B02U,0x2A6F2B94U,
    0xB40BBE37U,0xC30C8EA1U,0x5A05DF1BU,0x2D02EF8DU,
    };

    crc = crc ^ 0xFFFFFFFFU;
    while (len > 0) {
        crc = BAKELITE_FLASH_READ_32(table[*data ^ (uint8_t)crc]) ^ (crc >> 8);
        data++;
        len--;
    }
    crc = crc ^ 0xFFFFFFFFU;
    return crc;
}

#endif /* BAKELITE_COMMON_CRC_H */


/*
 * Bakelite COBS Functions (C99)
 *
 * COBS encode/decode functions are Copyright (c) 2010 Craig McQueen
 * Licensed under the MIT license (see end of file).
 * Source: https://github.com/cmcqueen/cobs-c
 */

#ifndef BAKELITE_COMMON_COBS_H
#define BAKELITE_COMMON_COBS_H

#include <stdint.h>
#include <stddef.h>

/* COBS buffer size calculations */
#define BAKELITE_COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN) ((SRC_LEN) + (((SRC_LEN) + 253u) / 254u))
#define BAKELITE_COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN) (((SRC_LEN) == 0) ? 0u : ((SRC_LEN) - 1u))
#define BAKELITE_COBS_OVERHEAD(BUF_SIZE)              (((BUF_SIZE) + 253u) / 254u)
#define BAKELITE_COBS_ENCODE_SRC_OFFSET(SRC_LEN)      (((SRC_LEN) + 253u) / 254u)

/* Legacy macro names for backward compatibility */
#define COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN) BAKELITE_COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)
#define COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN) BAKELITE_COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN)
#define COBS_ENCODE_SRC_OFFSET(SRC_LEN)      BAKELITE_COBS_ENCODE_SRC_OFFSET(SRC_LEN)

/* COBS encode/decode status */
typedef enum {
    BAKELITE_COBS_ENCODE_OK = 0x00,
    BAKELITE_COBS_ENCODE_NULL_POINTER = 0x01,
    BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW = 0x02
} Bakelite_CobsEncodeStatus;

typedef enum {
    BAKELITE_COBS_DECODE_OK = 0x00,
    BAKELITE_COBS_DECODE_NULL_POINTER = 0x01,
    BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW = 0x02,
    BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT = 0x04,
    BAKELITE_COBS_DECODE_INPUT_TOO_SHORT = 0x08
} Bakelite_CobsDecodeStatus;

/* COBS encode/decode results */
typedef struct {
    size_t out_len;
    int status;
} Bakelite_CobsEncodeResult;

typedef struct {
    size_t out_len;
    int status;
} Bakelite_CobsDecodeResult;

/*
 * COBS-encode a string of input bytes.
 *
 * dst_buf_ptr:    The buffer into which the result will be written
 * dst_buf_len:    Length of the buffer into which the result will be written
 * src_ptr:        The byte string to be encoded
 * src_len         Length of the byte string to be encoded
 *
 * returns:        A struct containing the success status of the encoding
 *                 operation and the length of the result (that was written to
 *                 dst_buf_ptr)
 */
static inline Bakelite_CobsEncodeResult bakelite_cobs_encode(void *dst_buf_ptr, size_t dst_buf_len,
                                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsEncodeResult result = {0, BAKELITE_COBS_ENCODE_OK};
    const uint8_t *src_read_ptr = (const uint8_t *)src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_code_write_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_write_ptr = dst_code_write_ptr + 1;
    uint8_t src_byte = 0;
    uint8_t search_len = 1;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = BAKELITE_COBS_ENCODE_NULL_POINTER;
        return result;
    }

    if (src_len != 0) {
        for (;;) {
            if (dst_write_ptr >= dst_buf_end_ptr) {
                result.status |= BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW;
                break;
            }

            src_byte = *src_read_ptr++;
            if (src_byte == 0) {
                *dst_code_write_ptr = search_len;
                dst_code_write_ptr = dst_write_ptr++;
                search_len = 1;
                if (src_read_ptr >= src_end_ptr) {
                    break;
                }
            } else {
                *dst_write_ptr++ = src_byte;
                search_len++;
                if (src_read_ptr >= src_end_ptr) {
                    break;
                }
                if (search_len == 0xFF) {
                    *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    search_len = 1;
                }
            }
        }
    }

    if (dst_code_write_ptr >= dst_buf_end_ptr) {
        result.status |= BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW;
        dst_write_ptr = dst_buf_end_ptr;
    } else {
        *dst_code_write_ptr = search_len;
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);
    return result;
}

/*
 * Decode a COBS byte string.
 *
 * dst_buf_ptr:    The buffer into which the result will be written
 * dst_buf_len:    Length of the buffer into which the result will be written
 * src_ptr:        The byte string to be decoded
 * src_len         Length of the byte string to be decoded
 *
 * returns:        A struct containing the success status of the decoding
 *                 operation and the length of the result (that was written to
 *                 dst_buf_ptr)
 */
static inline Bakelite_CobsDecodeResult bakelite_cobs_decode(void *dst_buf_ptr, size_t dst_buf_len,
                                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsDecodeResult result = {0, BAKELITE_COBS_DECODE_OK};
    const uint8_t *src_read_ptr = (const uint8_t *)src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_write_ptr = (uint8_t *)dst_buf_ptr;
    size_t remaining_bytes;
    uint8_t src_byte;
    uint8_t i;
    uint8_t len_code;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = BAKELITE_COBS_DECODE_NULL_POINTER;
        return result;
    }

    if (src_len != 0) {
        for (;;) {
            len_code = *src_read_ptr++;
            if (len_code == 0) {
                result.status |= BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT;
                break;
            }
            len_code--;

            remaining_bytes = (size_t)(src_end_ptr - src_read_ptr);
            if (len_code > remaining_bytes) {
                result.status |= BAKELITE_COBS_DECODE_INPUT_TOO_SHORT;
                len_code = (uint8_t)remaining_bytes;
            }

            remaining_bytes = (size_t)(dst_buf_end_ptr - dst_write_ptr);
            if (len_code > remaining_bytes) {
                result.status |= BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW;
                len_code = (uint8_t)remaining_bytes;
            }

            for (i = len_code; i != 0; i--) {
                src_byte = *src_read_ptr++;
                if (src_byte == 0) {
                    result.status |= BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT;
                }
                *dst_write_ptr++ = src_byte;
            }

            if (src_read_ptr >= src_end_ptr) {
                break;
            }

            if (len_code != 0xFE) {
                if (dst_write_ptr >= dst_buf_end_ptr) {
                    result.status |= BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW;
                    break;
                }
                *dst_write_ptr++ = 0;
            }
        }
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);
    return result;
}

/*
 * MIT License for COBS encode/decode functions:
 *
 * Copyright (c) 2010 Craig McQueen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#endif /* BAKELITE_COMMON_COBS_H */


/*
 * Type Definitions
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* Packed struct attribute for zero-copy mode */
#if defined(__GNUC__) || defined(__clang__)
  #define BAKELITE_PACKED __attribute__((packed))
#elif defined(_MSC_VER)
  #define BAKELITE_PACKED
  #pragma pack(push, 1)
#else
  #define BAKELITE_PACKED
#endif

/* Static assert for compile-time checks (C99 compatible) */
#define BAKELITE_STATIC_ASSERT(cond, msg) \
    typedef char bakelite_static_assert_##msg[(cond) ? 1 : -1]

/* Platform detection for unaligned access support */
#if defined(__AVR__) || defined(__arm__) || defined(__aarch64__) || \
    defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
  #define BAKELITE_UNALIGNED_OK 1
#else
  #define BAKELITE_UNALIGNED_OK 0
#endif

/* Error codes */
#define BAKELITE_OK              0
#define BAKELITE_ERR_WRITE      -1
#define BAKELITE_ERR_READ       -2
#define BAKELITE_ERR_SEEK       -3
#define BAKELITE_ERR_CAPACITY   -4  /* Data exceeds inline storage capacity */


/*
 * Buffer Stream
 */
/* Buffer stream for serialization/deserialization */
typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t pos;
} Bakelite_Buffer;

/* Initialize a buffer stream */
static inline void bakelite_buffer_init(Bakelite_Buffer *buf, uint8_t *data, uint32_t size) {
    buf->buffer = data;
    buf->size = size;
    buf->pos = 0;
}

/* Reset buffer position to start */
static inline void bakelite_buffer_reset(Bakelite_Buffer *buf) {
    buf->pos = 0;
}

/* Write data to buffer */
static inline int bakelite_buffer_write(Bakelite_Buffer *buf, const void *data, uint32_t length) {
    uint32_t end_pos = buf->pos + length;
    if (end_pos > buf->size) {
        return BAKELITE_ERR_WRITE;
    }
    memcpy(buf->buffer + buf->pos, data, length);
    buf->pos += length;
    return BAKELITE_OK;
}

/* Read data from buffer */
static inline int bakelite_buffer_read(Bakelite_Buffer *buf, void *data, uint32_t length) {
    uint32_t end_pos = buf->pos + length;
    if (end_pos > buf->size) {
        return BAKELITE_ERR_READ;
    }
    memcpy(data, buf->buffer + buf->pos, length);
    buf->pos += length;
    return BAKELITE_OK;
}

/* Seek to position */
static inline int bakelite_buffer_seek(Bakelite_Buffer *buf, uint32_t pos) {
    if (pos >= buf->size) {
        return BAKELITE_ERR_SEEK;
    }
    buf->pos = pos;
    return BAKELITE_OK;
}

/* Get current position */
static inline uint32_t bakelite_buffer_pos(const Bakelite_Buffer *buf) {
    return buf->pos;
}

/* Get buffer size */
static inline uint32_t bakelite_buffer_size(const Bakelite_Buffer *buf) {
    return buf->size;
}

/* Get remaining bytes */
static inline uint32_t bakelite_buffer_remaining(const Bakelite_Buffer *buf) {
    return buf->size - buf->pos;
}


/*
 * Serialization Functions
 */
/* Write primitives */
static inline int bakelite_write_bool(Bakelite_Buffer *buf, bool val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int8(Bakelite_Buffer *buf, int8_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint8(Bakelite_Buffer *buf, uint8_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int16(Bakelite_Buffer *buf, int16_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint16(Bakelite_Buffer *buf, uint16_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int32(Bakelite_Buffer *buf, int32_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint32(Bakelite_Buffer *buf, uint32_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int64(Bakelite_Buffer *buf, int64_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint64(Bakelite_Buffer *buf, uint64_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_float32(Bakelite_Buffer *buf, float val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_float64(Bakelite_Buffer *buf, double val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

/* Write variable-length bytes (length prefix + data) */
static inline int bakelite_write_bytes(Bakelite_Buffer *buf, const uint8_t *data, uint8_t len) {
    int rcode = bakelite_write_uint8(buf, len);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_buffer_write(buf, data, len);
}

/* Write null-terminated string */
static inline int bakelite_write_string(Bakelite_Buffer *buf, const char *val) {
    uint32_t len = (uint32_t)strlen(val);
    int rcode = bakelite_buffer_write(buf, val, len);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_write_uint8(buf, 0);  /* null terminator */
}

/* Read primitives */
static inline int bakelite_read_bool(Bakelite_Buffer *buf, bool *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int8(Bakelite_Buffer *buf, int8_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint8(Bakelite_Buffer *buf, uint8_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int16(Bakelite_Buffer *buf, int16_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint16(Bakelite_Buffer *buf, uint16_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int32(Bakelite_Buffer *buf, int32_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint32(Bakelite_Buffer *buf, uint32_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int64(Bakelite_Buffer *buf, int64_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint64(Bakelite_Buffer *buf, uint64_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_float32(Bakelite_Buffer *buf, float *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_float64(Bakelite_Buffer *buf, double *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

/* Read variable-length bytes into inline storage (length prefix + data) */
static inline int bakelite_read_bytes(Bakelite_Buffer *buf, uint8_t *data, uint8_t *len, uint8_t capacity) {
    uint8_t size;
    int rcode = bakelite_read_uint8(buf, &size);
    if (rcode != BAKELITE_OK) return rcode;

    if (size > capacity) {
        return BAKELITE_ERR_CAPACITY;
    }

    *len = size;
    return bakelite_buffer_read(buf, data, size);
}

/* Read null-terminated string into char array */
static inline int bakelite_read_string(Bakelite_Buffer *buf, char *val, uint32_t capacity) {
    uint32_t i = 0;
    while (i < capacity - 1) {
        int rcode = bakelite_buffer_read(buf, &val[i], 1);
        if (rcode != BAKELITE_OK) return rcode;
        if (val[i] == '\0') {
            return BAKELITE_OK;
        }
        i++;
    }
    /* Read and discard until null terminator or error */
    char c;
    do {
        int rcode = bakelite_buffer_read(buf, &c, 1);
        if (rcode != BAKELITE_OK) return rcode;
    } while (c != '\0');
    val[capacity - 1] = '\0';
    return BAKELITE_OK;
}

/* Write array of primitives */
#define BAKELITE_DEFINE_WRITE_ARRAY(type, name) \
    static inline int bakelite_write_array_##name(Bakelite_Buffer *buf, const type *arr, uint32_t count) { \
        for (uint32_t i = 0; i < count; i++) { \
            int rcode = bakelite_write_##name(buf, arr[i]); \
            if (rcode != BAKELITE_OK) return rcode; \
        } \
        return BAKELITE_OK; \
    }

BAKELITE_DEFINE_WRITE_ARRAY(bool, bool)
BAKELITE_DEFINE_WRITE_ARRAY(int8_t, int8)
BAKELITE_DEFINE_WRITE_ARRAY(uint8_t, uint8)
BAKELITE_DEFINE_WRITE_ARRAY(int16_t, int16)
BAKELITE_DEFINE_WRITE_ARRAY(uint16_t, uint16)
BAKELITE_DEFINE_WRITE_ARRAY(int32_t, int32)
BAKELITE_DEFINE_WRITE_ARRAY(uint32_t, uint32)
BAKELITE_DEFINE_WRITE_ARRAY(int64_t, int64)
BAKELITE_DEFINE_WRITE_ARRAY(uint64_t, uint64)
BAKELITE_DEFINE_WRITE_ARRAY(float, float32)
BAKELITE_DEFINE_WRITE_ARRAY(double, float64)

/* Read array of primitives */
#define BAKELITE_DEFINE_READ_ARRAY(type, name) \
    static inline int bakelite_read_array_##name(Bakelite_Buffer *buf, type *arr, uint32_t count) { \
        for (uint32_t i = 0; i < count; i++) { \
            int rcode = bakelite_read_##name(buf, &arr[i]); \
            if (rcode != BAKELITE_OK) return rcode; \
        } \
        return BAKELITE_OK; \
    }

BAKELITE_DEFINE_READ_ARRAY(bool, bool)
BAKELITE_DEFINE_READ_ARRAY(int8_t, int8)
BAKELITE_DEFINE_READ_ARRAY(uint8_t, uint8)
BAKELITE_DEFINE_READ_ARRAY(int16_t, int16)
BAKELITE_DEFINE_READ_ARRAY(uint16_t, uint16)
BAKELITE_DEFINE_READ_ARRAY(int32_t, int32)
BAKELITE_DEFINE_READ_ARRAY(uint32_t, uint32)
BAKELITE_DEFINE_READ_ARRAY(int64_t, int64)
BAKELITE_DEFINE_READ_ARRAY(uint64_t, uint64)
BAKELITE_DEFINE_READ_ARRAY(float, float32)
BAKELITE_DEFINE_READ_ARRAY(double, float64)


/*
 * CRC Functions
 */
/*
 * ctiny CRC - uses common C99 implementation
 * This file is intentionally minimal as the implementation is in common/crc.h
 */

/* CRC size constants (re-exported for convenience) */
#ifndef BAKELITE_CRC_NOOP_SIZE
#define BAKELITE_CRC_NOOP_SIZE 0
#define BAKELITE_CRC8_SIZE  1
#define BAKELITE_CRC16_SIZE 2
#define BAKELITE_CRC32_SIZE 4
#endif


/*
 * COBS Framing
 */
/*
 * ctiny COBS Framer - uses common C99 COBS implementation
 */

/* Calculate total buffer size needed for framer */
#define BAKELITE_FRAMER_BUFFER_SIZE(MAX_MSG_SIZE, CRC_SIZE) \
    (BAKELITE_COBS_OVERHEAD((MAX_MSG_SIZE) + (CRC_SIZE)) + (MAX_MSG_SIZE) + (CRC_SIZE) + 1)

/* Message offset in buffer (after COBS overhead) */
#define BAKELITE_FRAMER_MESSAGE_OFFSET(MAX_MSG_SIZE, CRC_SIZE) \
    BAKELITE_COBS_OVERHEAD((MAX_MSG_SIZE) + (CRC_SIZE))

/* Decode state enum */
typedef enum {
    BAKELITE_DECODE_OK = 0,
    BAKELITE_DECODE_NOT_READY,
    BAKELITE_DECODE_FAILURE,
    BAKELITE_DECODE_CRC_FAILURE,
    BAKELITE_DECODE_BUFFER_OVERRUN
} Bakelite_DecodeState;

/* Framer result */
typedef struct {
    int status;
    size_t length;
    uint8_t *data;
} Bakelite_FramerResult;

/* Decode result */
typedef struct {
    Bakelite_DecodeState status;
    size_t length;
    uint8_t *data;
} Bakelite_DecodeResult;

/* CRC type enum */
typedef enum {
    BAKELITE_CRC_NONE = 0,
    BAKELITE_CRC_8,
    BAKELITE_CRC_16,
    BAKELITE_CRC_32
} Bakelite_CrcType;

/* COBS Framer structure */
typedef struct {
    uint8_t *buffer;
    size_t buffer_size;
    size_t max_message_size;
    size_t message_offset;
    size_t crc_size;
    Bakelite_CrcType crc_type;
    uint8_t *read_pos;
} Bakelite_CobsFramer;

/* Get CRC size for a CRC type */
static inline size_t bakelite_crc_size(Bakelite_CrcType crc_type) {
    switch (crc_type) {
        case BAKELITE_CRC_8:  return 1;
        case BAKELITE_CRC_16: return 2;
        case BAKELITE_CRC_32: return 4;
        default: return 0;
    }
}

/* Initialize framer with user-provided buffer */
static inline void bakelite_framer_init(Bakelite_CobsFramer *framer,
                                         uint8_t *buffer, size_t buffer_size,
                                         size_t max_message_size,
                                         Bakelite_CrcType crc_type) {
    framer->buffer = buffer;
    framer->buffer_size = buffer_size;
    framer->max_message_size = max_message_size;
    framer->crc_type = crc_type;
    framer->crc_size = bakelite_crc_size(crc_type);
    framer->message_offset = BAKELITE_COBS_OVERHEAD(max_message_size + framer->crc_size);
    framer->read_pos = buffer;
}

/* Get pointer to message area in buffer */
static inline uint8_t *bakelite_framer_buffer(Bakelite_CobsFramer *framer) {
    return framer->buffer + framer->message_offset;
}

/* Get usable buffer size for messages */
static inline size_t bakelite_framer_buffer_size(Bakelite_CobsFramer *framer) {
    return framer->max_message_size + 1;  /* +1 for type byte */
}

/* Calculate and append CRC to data */
static inline void bakelite_framer_append_crc(Bakelite_CobsFramer *framer, uint8_t *data, size_t length) {
    switch (framer->crc_type) {
        case BAKELITE_CRC_8: {
            uint8_t crc = bakelite_crc8(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        case BAKELITE_CRC_16: {
            uint16_t crc = bakelite_crc16(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        case BAKELITE_CRC_32: {
            uint32_t crc = bakelite_crc32(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        default:
            break;
    }
}

/* Verify CRC of data */
static inline bool bakelite_framer_verify_crc(Bakelite_CobsFramer *framer, const uint8_t *data, size_t length) {
    switch (framer->crc_type) {
        case BAKELITE_CRC_8: {
            uint8_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc8(data, length, 0) == expected;
        }
        case BAKELITE_CRC_16: {
            uint16_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc16(data, length, 0) == expected;
        }
        case BAKELITE_CRC_32: {
            uint32_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc32(data, length, 0) == expected;
        }
        default:
            return true;
    }
}

/* Encode frame with data copy */
static inline Bakelite_FramerResult bakelite_framer_encode_copy(Bakelite_CobsFramer *framer,
                                                                  const uint8_t *data, size_t length) {
    uint8_t *msg_start = framer->buffer + framer->message_offset;
    memcpy(msg_start, data, length);

    if (framer->crc_size > 0) {
        bakelite_framer_append_crc(framer, msg_start, length);
    }

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(
        framer->buffer, framer->buffer_size,
        msg_start, length + framer->crc_size);

    if (result.status != 0) {
        return (Bakelite_FramerResult){ 1, 0, NULL };
    }

    framer->buffer[result.out_len] = 0;  /* Null terminator */
    return (Bakelite_FramerResult){ 0, result.out_len + 1, framer->buffer };
}

/* Encode frame (data already in buffer at message offset) */
static inline Bakelite_FramerResult bakelite_framer_encode(Bakelite_CobsFramer *framer, size_t length) {
    uint8_t *msg_start = framer->buffer + framer->message_offset;

    if (framer->crc_size > 0) {
        bakelite_framer_append_crc(framer, msg_start, length);
    }

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(
        framer->buffer, framer->buffer_size,
        msg_start, length + framer->crc_size);

    if (result.status != 0) {
        return (Bakelite_FramerResult){ 1, 0, NULL };
    }

    framer->buffer[result.out_len] = 0;  /* Null terminator */
    return (Bakelite_FramerResult){ 0, result.out_len + 1, framer->buffer };
}

/* Decode a complete frame */
static inline Bakelite_DecodeResult bakelite_framer_decode_frame(Bakelite_CobsFramer *framer, size_t length) {
    if (length == 1) {
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_FAILURE, 0, NULL };
    }

    length--;  /* Discard null byte */

    /* Decode in-place at buffer start */
    Bakelite_CobsDecodeResult result = bakelite_cobs_decode(
        framer->buffer, framer->buffer_size,
        framer->buffer, length);

    if (result.status != 0) {
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_FAILURE, 0, NULL };
    }

    /* Length of decoded data without CRC */
    length = result.out_len - framer->crc_size;

    if (framer->crc_size > 0) {
        if (!bakelite_framer_verify_crc(framer, framer->buffer, length)) {
            return (Bakelite_DecodeResult){ BAKELITE_DECODE_CRC_FAILURE, 0, NULL };
        }
    }

    /* Move decoded data to message offset position for consistent buffer layout */
    if (framer->message_offset > 0) {
        memmove(framer->buffer + framer->message_offset, framer->buffer, length);
    }

    return (Bakelite_DecodeResult){
        BAKELITE_DECODE_OK,
        length,
        framer->buffer + framer->message_offset
    };
}

/* Process a single byte from the stream */
static inline Bakelite_DecodeResult bakelite_framer_read_byte(Bakelite_CobsFramer *framer, uint8_t byte) {
    *framer->read_pos = byte;
    size_t length = (size_t)(framer->read_pos - framer->buffer) + 1;

    if (byte == 0) {
        framer->read_pos = framer->buffer;
        return bakelite_framer_decode_frame(framer, length);
    } else if (length == framer->buffer_size) {
        framer->read_pos = framer->buffer;
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_BUFFER_OVERRUN, 0, NULL };
    }

    framer->read_pos++;
    return (Bakelite_DecodeResult){ BAKELITE_DECODE_NOT_READY, 0, NULL };
}


/*
The MIT License
---------------

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#endif /* BAKELITE_H */
