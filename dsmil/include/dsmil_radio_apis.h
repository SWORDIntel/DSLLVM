/*
 * DSMIL Radio APIs Header
 *
 * This header defines the radio communication API functions for
 * SATCOM and military communication protocols.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_RADIO_APIS_H
#define DSMIL_RADIO_APIS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * SATCOM REED-SOLOMON ENCODING API
 * ============================================================================ */

/**
 * @brief Reed-Solomon forward error correction encoding for SATCOM
 *
 * This function applies Reed-Solomon forward error correction encoding
 * to satellite communication data for reliable transmission.
 *
 * @param data Input data buffer
 * @param data_len Length of input data in bytes
 * @param encoded Output buffer for encoded data (must be pre-allocated)
 * @param encoded_len Input: maximum encoded buffer size,
 *                   Output: actual encoded data length
 * @return 0 on success, negative error code on failure
 */
int dsmil_satcom_reed_solomon_encode(const uint8_t *data,
                                    size_t data_len,
                                    uint8_t *encoded,
                                    size_t *encoded_len);

/* ============================================================================
 * LINK-16 J-SERIES FORMATTING API
 * ============================================================================ */

/**
 * @brief Format message in Link-16 J-series format
 *
 * This function formats a message according to Link-16 J-series protocol
 * for military tactical data link communication.
 *
 * @param message Input message buffer
 * @param msg_len Length of input message in bytes
 * @param formatted Output buffer for formatted message (must be pre-allocated)
 * @param formatted_len Input: maximum formatted buffer size,
 *                     Output: actual formatted message length
 * @return 0 on success, negative error code on failure
 */
int dsmil_link16_format_j_series(const void *message,
                                size_t msg_len,
                                uint8_t *formatted,
                                size_t *formatted_len);

/* ============================================================================
 * RADIO API UTILITIES
 * ============================================================================ */

/**
 * @brief Check if radio protocol is supported
 *
 * @param protocol_type Type of radio protocol (0=SATCOM, 1=Link16)
 * @return 1 if supported, 0 if not supported
 */
int dsmil_radio_protocol_supported(int protocol_type);

/**
 * @brief Get radio protocol capabilities
 *
 * @param protocol_type Type of radio protocol
 * @param capabilities Pointer to capabilities structure (output)
 * @return 0 on success, negative error code on failure
 */
int dsmil_radio_get_capabilities(int protocol_type, void *capabilities);

/**
 * @brief Initialize radio protocol
 *
 * @param protocol_type Type of radio protocol to initialize
 * @return 0 on success, negative error code on failure
 */
int dsmil_radio_initialize(int protocol_type);

/**
 * @brief Cleanup radio protocol resources
 *
 * @param protocol_type Type of radio protocol to cleanup
 * @return 0 on success, negative error code on failure
 */
int dsmil_radio_cleanup(int protocol_type);

/* ============================================================================
 * RADIO PROTOCOL CONSTANTS
 * ============================================================================ */

/* SATCOM Reed-Solomon parameters */
#define SATCOM_RS_DATA_SYMBOLS 223  /* Data symbols per codeword */
#define SATCOM_RS_PARITY_SYMBOLS 32 /* Parity symbols per codeword */
#define SATCOM_RS_TOTAL_SYMBOLS (SATCOM_RS_DATA_SYMBOLS + SATCOM_RS_PARITY_SYMBOLS)

/* Link-16 J-series parameters */
#define LINK16_J_MAX_MESSAGE_SIZE 2048  /* Maximum message size in bytes */
#define LINK16_J_HEADER_SIZE 8          /* J-series header size */
#define LINK16_J_FOOTER_SIZE 4          /* J-series footer size */
#define LINK16_J_MAX_FORMATTED_SIZE (LINK16_J_HEADER_SIZE + LINK16_J_MAX_MESSAGE_SIZE + LINK16_J_FOOTER_SIZE)

/* Radio protocol types */
#define RADIO_PROTOCOL_SATCOM 0
#define RADIO_PROTOCOL_LINK16 1

#endif /* DSMIL_RADIO_APIS_H */
