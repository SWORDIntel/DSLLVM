/*
 * DSMIL TPM APIs Header
 *
 * This header defines the TPM (Trusted Platform Module) API functions for
 * PCR reading and quote generation as specified in model APIs.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_TPM_APIS_H
#define DSMIL_TPM_APIS_H

#include <stdint.h>
#include <stddef.h>

/* Forward declaration for Device 255 context */
typedef struct dsmil_device255_ctx dsmil_device255_ctx_t;

/* ============================================================================
 * TPM PCR READING API
 * ============================================================================ */

/**
 * @brief Read all 24 TPM PCR values
 *
 * Reads all 24 Platform Configuration Registers (PCRs) from the TPM.
 * Each PCR contains a SHA-256 hash (32 bytes).
 *
 * @param ctx Device 255 context (must be initialized)
 * @param pcr_values Output array of 24 PCR values (32 bytes each = SHA-256)
 * @return 0 on success, negative error code on failure
 */
int dsmil_device255_get_pcr_values(dsmil_device255_ctx_t *ctx,
                                   uint8_t pcr_values[24][32]);

/* ============================================================================
 * TPM QUOTE API
 * ============================================================================ */

/**
 * @brief Generate TPM2_Quote with attestation signature
 *
 * Generates a TPM2_Quote containing PCR digest and signature for remote
 * attestation using the provided PCR values.
 *
 * @param ctx Device 255 context (must be initialized)
 * @param nonce Nonce for quote freshness (typically 32 bytes)
 * @param pcr_values PCR values to include in quote (24 PCRs × 32 bytes each)
 * @param quote Output TPM2_Quote structure
 * @param quote_len Input: buffer size, Output: actual quote length
 * @return 0 on success, negative error code on failure
 */
int dsmil_device255_tpm_quote(dsmil_device255_ctx_t *ctx,
                              const uint8_t *nonce,
                              const uint8_t pcr_values[24][32],
                              uint8_t *quote,
                              size_t *quote_len);

/* ============================================================================
 * TPM API UTILITIES
 * ============================================================================ */

/**
 * @brief Check if TPM is available and functional
 *
 * @return 1 if TPM is available, 0 otherwise
 */
int dsmil_tpm_available(void);

/**
 * @brief Initialize TPM subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_tpm_initialize(void);

/**
 * @brief Cleanup TPM subsystem resources
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_tpm_cleanup(void);

/* ============================================================================
 * TPM CONSTANTS
 * ============================================================================ */

/* TPM PCR constants */
#define TPM_PCR_COUNT 24
#define TPM_PCR_SIZE 32  /* SHA-256 hash size */

/* TPM Quote constants */
#define TPM_QUOTE_MAX_SIZE 1024
#define TPM_NONCE_MAX_SIZE 32

#endif /* DSMIL_TPM_APIS_H */
