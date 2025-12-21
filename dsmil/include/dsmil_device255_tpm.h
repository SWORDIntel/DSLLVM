/**
 * @file dsmil_device255_tpm.h
 * @brief Device 255 TPM 2.0 Operations API
 * 
 * Production-grade API for TPM 2.0 operations including PCR reading
 * and quote generation for remote attestation.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_DEVICE255_TPM_H
#define DSMIL_DEVICE255_TPM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dsmil_device255_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup DSMIL_DEVICE255_TPM Device 255 TPM Operations
 * @{
 */

/**
 * @brief Read all 24 TPM PCR values
 * 
 * Reads all 24 Platform Configuration Registers (PCRs) from the TPM 2.0.
 * Each PCR contains a SHA-256 hash (32 bytes).
 * 
 * @param ctx Device 255 context (must be initialized)
 * @param pcr_values Output array of 24 PCR values (32 bytes each = SHA-256)
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 * - -EINVAL: Invalid parameters (NULL ctx or pcr_values)
 * - -ENODEV: TPM not available or not initialized
 * - -EIO: TPM communication error
 * - -ENOTSUP: PCR read not supported
 * 
 * @thread_safety Not thread-safe (TPM2-TSS contexts are not thread-safe)
 * @memory_ownership pcr_values is caller-owned, must be at least 24*32 bytes
 */
int dsmil_device255_get_pcr_values(dsmil_device255_ctx_t *ctx,
                                   uint8_t pcr_values[24][32]);

/**
 * @brief Generate TPM2_Quote for remote attestation
 * 
 * Generates a TPM2_Quote containing PCR digest and signature for remote
 * attestation. The quote is signed by the TPM using an Attestation Key (AK).
 * 
 * @param ctx Device 255 context (must be initialized)
 * @param nonce Nonce for freshness (typically 32 bytes, SHA-256 size)
 * @param nonce_len Length of nonce (must be <= 32)
 * @param pcr_selection PCR selection bitmap (bits 0-23 for PCRs 0-23)
 * @param quote Output buffer for quote (TPM2B_ATTEST structure)
 * @param quote_len Input: buffer size, Output: actual quote length
 * @param signature Output buffer for quote signature
 * @param signature_len Input: buffer size, Output: actual signature length
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 * - -EINVAL: Invalid parameters
 * - -ENODEV: TPM not available or not initialized
 * - -EIO: TPM communication error
 * - -ENOSPC: Quote or signature buffer too small
 * - -ENOTSUP: Quote generation not supported
 * 
 * @thread_safety Not thread-safe
 * @memory_ownership All buffers are caller-owned
 */
int dsmil_device255_tpm_quote(dsmil_device255_ctx_t *ctx,
                              const uint8_t *nonce,
                              size_t nonce_len,
                              uint32_t pcr_selection,
                              uint8_t *quote,
                              size_t *quote_len,
                              uint8_t *signature,
                              size_t *signature_len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_DEVICE255_TPM_H */

