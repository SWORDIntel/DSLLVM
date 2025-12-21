/*
 * DSMIL TPM PCR Runtime Implementation
 *
 * This file implements the TPM PCR reading functionality as specified
 * in the model APIs for Device 255 integration.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_tpm_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tpm.h>

/* Forward declarations for TPM device operations */
extern struct tpm_chip *dsmil_get_tpm_chip(void);
extern int dsmil_tpm_pcr_read(struct tpm_chip *chip, int pcr_idx,
                             u8 *digest, size_t digest_len);

/**
 * @brief Validate TPM PCR read parameters
 *
 * @param ctx Device context
 * @param pcr_values Output buffer
 * @return 0 if valid, negative error code otherwise
 */
static int validate_pcr_params(dsmil_device255_ctx_t *ctx,
                              uint8_t pcr_values[24][32])
{
    if (!ctx) {
        pr_err("dsmil: TPM PCR read: Invalid context\n");
        return -EINVAL;
    }

    if (!pcr_values) {
        pr_err("dsmil: TPM PCR read: Invalid PCR values buffer\n");
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Read all 24 TPM PCR values
 *
 * Implements the model API specification for reading all TPM PCRs.
 */
int dsmil_device255_get_pcr_values(dsmil_device255_ctx_t *ctx,
                                   uint8_t pcr_values[24][32])
{
    struct tpm_chip *chip;
    int ret, i;

    /* Validate parameters */
    ret = validate_pcr_params(ctx, pcr_values);
    if (ret != 0)
        return ret;

    /* Check TPM availability */
    if (!dsmil_tpm_available()) {
        pr_err("dsmil: TPM PCR read: TPM not available\n");
        return -ENODEV;
    }

    /* Get TPM chip */
    chip = dsmil_get_tpm_chip();
    if (!chip) {
        pr_err("dsmil: TPM PCR read: Failed to get TPM chip\n");
        return -ENODEV;
    }

    /* Read all 24 PCRs */
    for (i = 0; i < TPM_PCR_COUNT; i++) {
        ret = dsmil_tpm_pcr_read(chip, i, pcr_values[i], TPM_PCR_SIZE);
        if (ret != 0) {
            pr_err("dsmil: TPM PCR read: Failed to read PCR %d: %d\n", i, ret);
            return ret;
        }
    }

    pr_debug("dsmil: TPM PCR read: Successfully read all %d PCRs\n", TPM_PCR_COUNT);
    return 0;
}

/**
 * @brief Check if TPM is available
 *
 * @return 1 if TPM is available, 0 otherwise
 */
int dsmil_tpm_available(void)
{
    struct tpm_chip *chip;

    chip = dsmil_get_tpm_chip();
    return (chip != NULL) ? 1 : 0;
}

/**
 * @brief Initialize TPM subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_tpm_initialize(void)
{
    /* TPM initialization is handled by the kernel TPM subsystem */
    /* This function can be used for any additional DSMIL-specific setup */

    if (!dsmil_tpm_available()) {
        pr_err("dsmil: TPM initialize: TPM not available\n");
        return -ENODEV;
    }

    pr_info("dsmil: TPM subsystem initialized\n");
    return 0;
}

/**
 * @brief Cleanup TPM subsystem resources
 *
 * @return 0 on success, negative error code on failure
 */
int dsmil_tpm_cleanup(void)
{
    /* TPM cleanup is handled by the kernel TPM subsystem */
    /* This function can be used for any additional DSMIL-specific cleanup */

    pr_info("dsmil: TPM subsystem cleanup completed\n");
    return 0;
}

/*
 * TPM PCR Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
