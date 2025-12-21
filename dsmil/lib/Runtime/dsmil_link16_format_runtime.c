/*
 * DSMIL Link-16 J-Series Runtime Implementation
 *
 * This file implements the Link-16 J-series message formatting
 * for military tactical data link communication.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_radio_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/crc32.h>

/* Link-16 J-series protocol constants */
#define LINK16_J_SYNC_WORD 0xFE6B2840  /* J-series sync word */
#define LINK16_J_PROTOCOL_VERSION 0x01  /* Protocol version */
#define LINK16_J_MESSAGE_TYPE_DATA 0x01 /* Data message type */
#define LINK16_J_MESSAGE_TYPE_ACK 0x02  /* Acknowledgment type */
#define LINK16_J_MESSAGE_TYPE_NAK 0x03  /* Negative acknowledgment type */

/* J-series header structure */
struct link16_j_header {
    uint32_t sync_word;        /* Synchronization word */
    uint8_t protocol_version;  /* Protocol version */
    uint8_t message_type;      /* Message type */
    uint16_t message_length;   /* Message length (network byte order) */
    uint32_t sequence_number;  /* Sequence number (network byte order) */
    uint32_t timestamp;        /* Timestamp (network byte order) */
};

/* J-series footer structure */
struct link16_j_footer {
    uint32_t crc32;            /* CRC-32 checksum */
};

/**
 * @brief Convert to network byte order (big-endian)
 *
 * @param value 16-bit value to convert
 * @return Value in network byte order
 */
static uint16_t hton16(uint16_t value)
{
    return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

/**
 * @brief Convert to network byte order (big-endian)
 *
 * @param value 32-bit value to convert
 * @return Value in network byte order
 */
static uint32_t hton32(uint32_t value)
{
    return ((value & 0xFF) << 24) |
           ((value & 0xFF00) << 8) |
           ((value & 0xFF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
}

/**
 * @brief Validate Link-16 J-series parameters
 *
 * @param message Input message buffer
 * @param msg_len Input message length
 * @param formatted Output buffer
 * @param formatted_len Formatted buffer length pointer
 * @return 0 if valid, negative error code otherwise
 */
static int validate_link16_j_params(const void *message,
                                   size_t msg_len,
                                   uint8_t *formatted,
                                   size_t *formatted_len)
{
    size_t required_len;

    if (!message || !formatted || !formatted_len)
        return -EINVAL;

    if (msg_len == 0 || msg_len > LINK16_J_MAX_MESSAGE_SIZE)
        return -EINVAL;

    /* Calculate required formatted length */
    required_len = sizeof(struct link16_j_header) + msg_len + sizeof(struct link16_j_footer);

    if (*formatted_len < required_len) {
        *formatted_len = required_len;
        return -ENOBUFS;
    }

    return 0;
}

/**
 * @brief Calculate CRC-32 for Link-16 message
 *
 * @param data Data buffer
 * @param length Data length
 * @return CRC-32 checksum
 */
static uint32_t link16_j_crc32(const uint8_t *data, size_t length)
{
    return crc32_le(~0, data, length) ^ ~0;
}

/**
 * @brief Get current timestamp for Link-16
 *
 * @return Timestamp in seconds since epoch
 */
static uint32_t link16_j_timestamp(void)
{
    /* In kernel space, use current kernel time */
    struct timespec64 ts;
    ktime_get_real_ts64(&ts);
    return (uint32_t)ts.tv_sec;
}

/**
 * @brief Generate sequence number for Link-16 message
 *
 * @return Sequence number (simple incrementing counter)
 */
static uint32_t link16_j_sequence_number(void)
{
    static atomic_t sequence_counter = ATOMIC_INIT(0);
    return atomic_inc_return(&sequence_counter);
}

/**
 * @brief Link-16 J-series message formatting implementation
 *
 * Formats a message according to Link-16 J-series protocol specifications
 * for military tactical data link communication.
 */
int dsmil_link16_format_j_series(const void *message,
                                size_t msg_len,
                                uint8_t *formatted,
                                size_t *formatted_len)
{
    struct link16_j_header *header;
    struct link16_j_footer *footer;
    uint8_t *payload;
    size_t total_len;
    int ret;

    /* Validate parameters */
    ret = validate_link16_j_params(message, msg_len, formatted, formatted_len);
    if (ret != 0)
        return ret;

    /* Calculate total formatted length */
    total_len = sizeof(struct link16_j_header) + msg_len + sizeof(struct link16_j_footer);

    /* Set up pointers */
    header = (struct link16_j_header *)formatted;
    payload = formatted + sizeof(struct link16_j_header);
    footer = (struct link16_j_footer *)(formatted + sizeof(struct link16_j_header) + msg_len);

    /* Fill header */
    header->sync_word = hton32(LINK16_J_SYNC_WORD);
    header->protocol_version = LINK16_J_PROTOCOL_VERSION;
    header->message_type = LINK16_J_MESSAGE_TYPE_DATA;
    header->message_length = hton16((uint16_t)msg_len);
    header->sequence_number = hton32(link16_j_sequence_number());
    header->timestamp = hton32(link16_j_timestamp());

    /* Copy message payload */
    memcpy(payload, message, msg_len);

    /* Calculate and fill CRC */
    footer->crc32 = hton32(link16_j_crc32(formatted,
                                         sizeof(struct link16_j_header) + msg_len));

    /* Update output length */
    *formatted_len = total_len;

    return 0;
}

/**
 * @brief Check if Link-16 protocol is supported
 *
 * @return 1 if supported, 0 otherwise
 */
static int link16_supported(void)
{
    /* Link-16 J-series is always supported as it's protocol-based */
    return 1;
}

/**
 * @brief Get Link-16 capabilities
 *
 * @param capabilities Pointer to capabilities structure
 * @return 0 on success, negative error code on failure
 */
static int get_link16_capabilities(void *capabilities)
{
    /* Link-16 capabilities would include:
     * - Maximum message size
     * - Supported message types
     * - Protocol version
     * - Timing parameters
     */
    return -ENOSYS; /* Not implemented yet */
}

/**
 * @brief Initialize Link-16 protocol
 *
 * @return 0 on success, negative error code on failure
 */
static int link16_initialize(void)
{
    /* Initialize sequence counter */
    atomic_set(&((atomic_t){ATOMIC_INIT(0)}), 0);

    return 0;
}

/**
 * @brief Cleanup Link-16 protocol resources
 *
 * @return 0 on success, negative error code on failure
 */
static int link16_cleanup(void)
{
    /* No cleanup needed for Link-16 protocol state */
    return 0;
}

/**
 * @brief Check if radio protocol is supported (Link-16 implementation)
 */
int dsmil_radio_protocol_supported(int protocol_type)
{
    if (protocol_type == RADIO_PROTOCOL_LINK16)
        return link16_supported();

    return 0;
}

/**
 * @brief Get radio protocol capabilities (Link-16 implementation)
 */
int dsmil_radio_get_capabilities(int protocol_type, void *capabilities)
{
    if (protocol_type == RADIO_PROTOCOL_LINK16)
        return get_link16_capabilities(capabilities);

    return -EINVAL;
}

/**
 * @brief Initialize radio protocol (Link-16 implementation)
 */
int dsmil_radio_initialize(int protocol_type)
{
    if (protocol_type == RADIO_PROTOCOL_LINK16)
        return link16_initialize();

    return -EINVAL;
}

/**
 * @brief Cleanup radio protocol resources (Link-16 implementation)
 */
int dsmil_radio_cleanup(int protocol_type)
{
    if (protocol_type == RADIO_PROTOCOL_LINK16)
        return link16_cleanup();

    return -EINVAL;
}

/*
 * Link-16 J-Series Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
