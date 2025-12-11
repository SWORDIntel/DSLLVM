/**
 * @file dsmil_layer8_security_crypto_runtime.h
 * @brief Layer 8 (ENHANCED_SEC) Security Crypto Runtime API
 * 
 * Provides runtime interface for Layer 8 PQC-only crypto enforcement:
 * - PQC-only mode enablement
 * - PQC algorithm verification
 * - Crypto compliance auditing
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_LAYER8_SECURITY_CRYPTO_RUNTIME_H
#define DSMIL_LAYER8_SECURITY_CRYPTO_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup DSMIL_LAYER8_CRYPTO Layer 8 Security Crypto Runtime
 * @{
 */

/**
 * @brief Enable PQC-only mode (disable classical crypto)
 * 
 * Enforces Layer 8 requirement for post-quantum cryptography only.
 * Disables classical asymmetric (RSA) and ECC algorithms via Device 255.
 * 
 * @return 0 on success, negative on error
 */
int dsmil_layer8_enable_pqc_only_mode(void);

/**
 * @brief Verify that only PQC algorithms are used
 * 
 * Checks if the given algorithm is a post-quantum algorithm.
 * Returns false for classical algorithms (RSA, ECDSA) which should be disabled.
 * 
 * @param algorithm Algorithm ID to verify (TPM_ALG_*)
 * @return true if PQC algorithm, false if classical
 */
bool dsmil_layer8_verify_pqc_algorithm(uint16_t algorithm);

/**
 * @brief Audit crypto operations for PQC compliance
 * 
 * Retrieves statistics from Device 255 and reports:
 * - Total crypto operations
 * - PQC operations count
 * - Classical operations count (should be 0 in PQC-only mode)
 * 
 * @param total_ops Output total operations (may be NULL)
 * @param pqc_ops Output PQC operations count (may be NULL)
 * @param classical_ops Output classical operations count (may be NULL, should be 0)
 * @return 0 on success, negative on error
 */
int dsmil_layer8_audit_crypto_compliance(uint64_t *total_ops,
                                        uint64_t *pqc_ops,
                                        uint64_t *classical_ops);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_LAYER8_SECURITY_CRYPTO_RUNTIME_H */

