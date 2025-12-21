/*
 * DSMIL PKI APIs Header
 *
 * This header defines the PKI (Public Key Infrastructure) API functions
 * for certificate verification and cryptographic operations.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_PKI_APIS_H
#define DSMIL_PKI_APIS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * PKI CERTIFICATE VERIFICATION API
 * ============================================================================ */

/**
 * @brief Verify X.509 certificate chain
 *
 * This function verifies an X.509 certificate against a Certificate Authority
 * certificate for partner authentication in military communication systems.
 *
 * @param cert Certificate to verify (DER format)
 * @param cert_len Length of certificate in bytes
 * @param ca_cert CA certificate for verification (DER format)
 * @param ca_cert_len Length of CA certificate in bytes
 * @param is_valid Output: true if certificate is valid, false otherwise
 * @return 0 on successful verification check, negative error code on failure
 */
int dsmil_pki_verify_certificate(const uint8_t *cert,
                                size_t cert_len,
                                const uint8_t *ca_cert,
                                size_t ca_cert_len,
                                bool *is_valid);

/* ============================================================================
 * PKI API UTILITIES
 * ============================================================================ */

/**
 * @brief Check if PKI subsystem is available
 *
 * @return 1 if PKI is available, 0 otherwise
 */
int dsmil_pki_available(void);

/**
 * @brief Initialize PKI subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_pki_initialize(void);

/**
 * @brief Cleanup PKI subsystem resources
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_pki_cleanup(void);

/**
 * @brief Get supported certificate formats
 *
 * @return Bitmask of supported formats (1=DER, 2=PEM)
 */
int dsmil_pki_get_supported_formats(void);

/* ============================================================================
 * PKI CONSTANTS
 * ============================================================================ */

/* Certificate formats */
#define PKI_FORMAT_DER 1    /* Distinguished Encoding Rules */
#define PKI_FORMAT_PEM 2    /* Privacy Enhanced Mail */

/* Certificate validation results */
#define PKI_CERT_VALID 0
#define PKI_CERT_INVALID -1
#define PKI_CERT_EXPIRED -2
#define PKI_CERT_REVOKED -3
#define PKI_CERT_UNTRUSTED -4
#define PKI_CERT_MALFORMED -5

/* Maximum certificate sizes */
#define PKI_MAX_CERT_SIZE 8192    /* Maximum certificate size in bytes */
#define PKI_MAX_CHAIN_DEPTH 10    /* Maximum certificate chain depth */

#endif /* DSMIL_PKI_APIS_H */
