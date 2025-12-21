/**
 * @file dsmil_device255_tpm_quote_runtime.c
 * @brief TPM2_Quote Implementation
 * 
 * Implements dsmil_device255_tpm_quote() for generating TPM2_Quote with
 * PCR digest and attestation signature.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_device255_tpm.h"
#include "dsmil_device255_crypto.h"
#include <tss2/tss2_sys.h>
#include <tss2/tss2_tpm_types.h>
#include <tss2/tss2_mu.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Forward declarations
 */
extern TSS2_SYS_CONTEXT* dsmil_device255_get_tpm_context(void);
extern int dsmil_tpm_translate_error(TSS2_RC tpm_rc);
extern bool dsmil_tpm_is_available(void);

/**
 * @brief Generate TPM2_Quote
 */
int dsmil_device255_tpm_quote(dsmil_device255_ctx_t *ctx,
                              const uint8_t *nonce,
                              size_t nonce_len,
                              uint32_t pcr_selection,
                              uint8_t *quote,
                              size_t *quote_len,
                              uint8_t *signature,
                              size_t *signature_len) {
    if (!ctx || !nonce || !quote || !quote_len || !signature || !signature_len) {
        fprintf(stderr, "ERROR: Invalid parameters to dsmil_device255_tpm_quote\n");
        return -EINVAL;
    }
    
    if (nonce_len == 0 || nonce_len > 32) {
        fprintf(stderr, "ERROR: Invalid nonce length: %zu (must be 1-32)\n", nonce_len);
        return -EINVAL;
    }
    
    // Check if TPM is available
    if (!dsmil_tpm_is_available()) {
        fprintf(stderr, "ERROR: TPM not available\n");
        return -ENODEV;
    }
    
    // Get TPM2-TSS system context
    TSS2_SYS_CONTEXT* sys_ctx = dsmil_device255_get_tpm_context();
    if (!sys_ctx) {
        fprintf(stderr, "ERROR: Failed to get TPM context\n");
        return -ENODEV;
    }
    
    // Prepare PCR selection from bitmap
    TPML_PCR_SELECTION pcr_selection_in = {0};
    pcr_selection_in.count = 1;
    pcr_selection_in.pcrSelections[0].hash = TPM2_ALG_SHA256;
    pcr_selection_in.pcrSelections[0].sizeofSelect = 3;  // 3 bytes for 24 PCRs
    
    // Convert pcr_selection bitmap to pcrSelect array
    for (int i = 0; i < 24; i++) {
        if (pcr_selection & (1U << i)) {
            int byte_idx = i / 8;
            int bit_idx = i % 8;
            pcr_selection_in.pcrSelections[0].pcrSelect[byte_idx] |= (1U << bit_idx);
        }
    }
    
    // Prepare nonce
    TPM2B_NONCE qualifying_data = {0};
    if (nonce_len > sizeof(qualifying_data.buffer)) {
        nonce_len = sizeof(qualifying_data.buffer);
    }
    qualifying_data.size = nonce_len;
    memcpy(qualifying_data.buffer, nonce, nonce_len);
    
    // Prepare signing scheme
    TPMT_SIG_SCHEME in_scheme = {0};
    in_scheme.scheme = TPM2_ALG_RSASSA;
    in_scheme.details.rsassa.hashAlg = TPM2_ALG_SHA256;
    
    // Prepare key handle (use NULL handle for default Attestation Key)
    // In production, this would use a properly loaded Attestation Key
    TPMI_DH_OBJECT key_handle = TPM2_RH_NULL;
    
    // Prepare output buffers
    TPM2B_ATTEST quoted = {0};
    TPMS_ATTEST attest_struct = {0};
    TPMT_SIGNATURE signature_out = {0};
    
    // Call TPM2_Quote
    TSS2_RC rc = Tss2_Sys_Quote(
        sys_ctx,
        NULL,  // No authorization session for key
        key_handle,
        &qualifying_data,
        &in_scheme,
        &pcr_selection_in,
        &quoted,
        &signature_out,
        NULL  // No response authorization
    );
    
    if (rc != TSS2_RC_SUCCESS) {
        int dsmil_error = dsmil_tpm_translate_error(rc);
        fprintf(stderr, "ERROR: TPM2_Quote failed: 0x%08X\n", rc);
        return dsmil_error;
    }
    
    // Serialize quote (TPM2B_ATTEST) to output buffer
    size_t required_quote_len = quoted.size;
    if (*quote_len < required_quote_len) {
        fprintf(stderr, "ERROR: Quote buffer too small: need %zu, got %zu\n",
                required_quote_len, *quote_len);
        *quote_len = required_quote_len;
        return -ENOSPC;
    }
    
    memcpy(quote, quoted.buffer, quoted.size);
    *quote_len = quoted.size;
    
    // Extract signature
    // TPMT_SIGNATURE contains the signature in format-specific structure
    size_t required_sig_len = 0;
    if (signature_out.sigAlg == TPM2_ALG_RSASSA) {
        // RSASSA signature: size depends on key size (typically 256 bytes for RSA-2048)
        required_sig_len = signature_out.signature.rsassa.sig.size;
        if (*signature_len < required_sig_len) {
            fprintf(stderr, "ERROR: Signature buffer too small: need %zu, got %zu\n",
                    required_sig_len, *signature_len);
            *signature_len = required_sig_len;
            return -ENOSPC;
        }
        memcpy(signature, signature_out.signature.rsassa.sig.buffer, required_sig_len);
        *signature_len = required_sig_len;
    } else if (signature_out.sigAlg == TPM2_ALG_ECDSA) {
        // ECDSA signature: R and S values
        required_sig_len = signature_out.signature.ecdsa.signatureR.size +
                          signature_out.signature.ecdsa.signatureS.size;
        if (*signature_len < required_sig_len) {
            fprintf(stderr, "ERROR: Signature buffer too small: need %zu, got %zu\n",
                    required_sig_len, *signature_len);
            *signature_len = required_sig_len;
            return -ENOSPC;
        }
        // Concatenate R and S
        memcpy(signature, signature_out.signature.ecdsa.signatureR.buffer,
               signature_out.signature.ecdsa.signatureR.size);
        memcpy(signature + signature_out.signature.ecdsa.signatureR.size,
               signature_out.signature.ecdsa.signatureS.buffer,
               signature_out.signature.ecdsa.signatureS.size);
        *signature_len = required_sig_len;
    } else {
        fprintf(stderr, "ERROR: Unsupported signature algorithm: 0x%04X\n",
                signature_out.sigAlg);
        return -ENOTSUP;
    }
    
    fprintf(stdout, "INFO: TPM2_Quote generated successfully (quote: %zu bytes, signature: %zu bytes)\n",
            *quote_len, *signature_len);
    
    return 0;
}

