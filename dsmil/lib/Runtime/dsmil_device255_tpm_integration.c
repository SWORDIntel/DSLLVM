/**
 * @file dsmil_device255_tpm_integration.c
 * @brief TPM2-TSS Integration Layer
 * 
 * Low-level wrapper for TPM2-TSS library providing context management,
 * error translation, and thread safety.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_device255_tpm.h"
#include "dsmil_device255_crypto.h"
#include <tss2/tss2_sys.h>
#include <tss2/tss2_tcti.h>
#include <tss2/tss2_tcti_device.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

/**
 * @brief Forward declaration for Device 255 TPM context accessor
 */
extern TSS2_SYS_CONTEXT* dsmil_device255_get_tpm_context(void);

/**
 * @brief Translate TPM2 error code to DSMIL error code
 * 
 * @param tpm_rc TPM2 return code
 * @return DSMIL error code (negative)
 */
int dsmil_tpm_translate_error(TSS2_RC tpm_rc) {
    if (tpm_rc == TSS2_RC_SUCCESS) {
        return 0;
    }
    
    // Extract error layer and code
    TSS2_RC error_code = TSS2_RC_LAYER(tpm_rc);
    TSS2_RC error_number = TSS2_RC_ERROR(tpm_rc);
    
    fprintf(stderr, "ERROR: TPM2 error [layer=0x%02X, code=0x%04X]: 0x%08X\n",
            error_code, error_number, tpm_rc);
    
    // Map common TPM2 errors to DSMIL error codes
    if (error_code == TSS2_TPM_RC_LAYER) {
        switch (error_number) {
            case TPM_RC_SUCCESS:
                return 0;
            case TPM_RC_BAD_PARAMETER:
            case TPM_RC_SIZE:
                return -EINVAL;
            case TPM_RC_MEMORY:
                return -ENOMEM;
            case TPM_RC_HANDLE:
            case TPM_RC_ATTRIBUTES:
                return -ENODEV;
            case TPM_RC_INSUFFICIENT:
                return -ENOSPC;
            case TPM_RC_COMMAND_CODE:
            case TPM_RC_AUTH_FAIL:
                return -EACCES;
            default:
                return -EIO;
        }
    } else if (error_code == TSS2_SYS_RC_LAYER) {
        switch (error_number) {
            case TSS2_SYS_RC_BAD_REFERENCE:
            case TSS2_SYS_RC_BAD_SIZE:
                return -EINVAL;
            case TSS2_SYS_RC_INSUFFICIENT_CONTEXT:
            case TSS2_SYS_RC_INSUFFICIENT_RESPONSE:
                return -ENOSPC;
            case TSS2_SYS_RC_BAD_SEQUENCE:
                return -EIO;
            default:
                return -EIO;
        }
    } else if (error_code == TSS2_TCTI_RC_LAYER) {
        switch (error_number) {
            case TSS2_TCTI_RC_BAD_CONTEXT:
            case TSS2_TCTI_RC_BAD_PARAMETER:
                return -EINVAL;
            case TSS2_TCTI_RC_NOT_IMPLEMENTED:
                return -ENOTSUP;
            case TSS2_TCTI_RC_IO_ERROR:
            case TSS2_TCTI_RC_GENERAL_FAILURE:
                return -EIO;
            default:
                return -EIO;
        }
    }
    
    // Default to I/O error for unknown errors
    return -EIO;
}

/**
 * @brief Check if TPM is available
 * 
 * @return true if TPM is available, false otherwise
 */
bool dsmil_tpm_is_available(void) {
    TSS2_SYS_CONTEXT* sys_ctx = dsmil_device255_get_tpm_context();
    return (sys_ctx != NULL);
}

