/**
 * @file dsmil_device255_pcr_runtime.c
 * @brief TPM PCR Reading Implementation
 * 
 * Implements dsmil_device255_get_pcr_values() for reading all 24 TPM PCRs
 * using TPM2_PCR_Read command.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_device255_tpm.h"
#include "dsmil_device255_crypto.h"
#include <tss2/tss2_sys.h>
#include <tss2/tss2_tpm_types.h>
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
 * @brief Read all 24 TPM PCR values
 */
int dsmil_device255_get_pcr_values(dsmil_device255_ctx_t *ctx,
                                   uint8_t pcr_values[24][32]) {
    if (!ctx || !pcr_values) {
        fprintf(stderr, "ERROR: Invalid parameters to dsmil_device255_get_pcr_values\n");
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
    
    // Prepare PCR selection: All 24 PCRs (0-23)
    TPML_PCR_SELECTION pcr_selection = {0};
    pcr_selection.count = 1;
    pcr_selection.pcrSelections[0].hash = TPM2_ALG_SHA256;
    pcr_selection.pcrSelections[0].sizeofSelect = 3;  // 3 bytes for 24 PCRs
    // Set all bits for PCRs 0-23
    pcr_selection.pcrSelections[0].pcrSelect[0] = 0xFF;  // PCRs 0-7
    pcr_selection.pcrSelections[0].pcrSelect[1] = 0xFF;  // PCRs 8-15
    pcr_selection.pcrSelections[0].pcrSelect[2] = 0xFF;  // PCRs 16-23
    
    // Prepare command buffer
    TPM2B_MAX_BUFFER pcr_update_counter = {0};
    TPML_PCR_SELECTION pcr_selection_out = {0};
    TPML_DIGEST pcr_values_out = {0};
    
    // Call TPM2_PCR_Read
    TSS2_RC rc = Tss2_Sys_PCR_Read(
        sys_ctx,
        NULL,  // No authorization sessions
        &pcr_selection,
        &pcr_update_counter,
        &pcr_selection_out,
        &pcr_values_out,
        NULL  // No response authorization
    );
    
    if (rc != TSS2_RC_SUCCESS) {
        int dsmil_error = dsmil_tpm_translate_error(rc);
        fprintf(stderr, "ERROR: TPM2_PCR_Read failed: 0x%08X\n", rc);
        return dsmil_error;
    }
    
    // Extract PCR values from response
    // pcr_values_out contains digests in PCR order
    if (pcr_values_out.count != 24) {
        fprintf(stderr, "ERROR: Expected 24 PCR values, got %u\n", pcr_values_out.count);
        return -EIO;
    }
    
    // Copy PCR values to output array
    for (uint32_t i = 0; i < 24 && i < pcr_values_out.count; i++) {
        if (pcr_values_out.digests[i].size != 32) {
            fprintf(stderr, "ERROR: PCR %u has invalid size: %u (expected 32)\n",
                    i, pcr_values_out.digests[i].size);
            return -EIO;
        }
        memcpy(pcr_values[i], pcr_values_out.digests[i].buffer, 32);
    }
    
    fprintf(stdout, "INFO: Successfully read 24 PCR values from TPM\n");
    
    return 0;
}

