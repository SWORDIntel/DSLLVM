/*
 * DSMIL SMC Call Runtime Implementation
 *
 * This file implements the SMC (Secure Monitor Call) functionality for
 * ARM TrustZone secure world operations.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#include "dsmil_enclave_apis.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <asm/cpufeature.h>
#include <asm/cputype.h>

/* ARM SMC calling convention */
#define SMC_CALL_NUM 0x82000000  /* Fast call */
#define SMC_CALL_NUM_STD 0x84000000  /* Standard call */
#define SMC_CALL_NUM_FAST 0x80000000  /* Fast call alternative */

/* SMC function IDs for common operations */
#define SMC_FUNC_TRUSTZONE_INFO 0x100
#define SMC_FUNC_CRYPTO_OPERATIONS 0x200
#define SMC_FUNC_SECURE_STORAGE 0x300
#define SMC_FUNC_SECURE_BOOT 0x400

/* TrustZone secure world status codes */
#define TZ_SUCCESS 0
#define TZ_ERROR_INVALID_PARAMS -1
#define TZ_ERROR_NOT_SUPPORTED -2
#define TZ_ERROR_BUSY -3
#define TZ_ERROR_TIMEOUT -4
#define TZ_ERROR_ACCESS_DENIED -5

/**
 * @brief Check if ARM TrustZone is supported
 *
 * @return 1 if TrustZone is supported, 0 otherwise
 */
static int trustZone_supported(void)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
    /* Check CPU ID for TrustZone support */
    uint32_t midr = read_cpuid_id();

    /* TrustZone is supported on ARMv7 and later with security extensions */
    if ((midr & 0xFF000000) >= 0x41000000) { /* ARM architecture */
        /* Check for security extensions */
        uint32_t id_pfr1 = read_cpuid_ext(CPUID_EXT_PFR1);
        if (id_pfr1 & (1 << 4)) { /* Security extension supported */
            return 1;
        }
    }

    return 0;
#else
    /* TrustZone not supported on non-ARM architectures */
    return 0;
#endif
}

/**
 * @brief Check if we're in secure world
 *
 * @return 1 if in secure world, 0 if in normal world
 */
static int in_secure_world(void)
{
#ifdef CONFIG_ARM64
    uint64_t scr;

    /* Read Secure Configuration Register (ARM64) */
    asm volatile("mrs %0, scr_el3" : "=r" (scr));

    /* Check NS bit (Non-Secure bit) */
    return !(scr & (1ULL << 0));
#elif defined(CONFIG_ARM)
    uint32_t scr;

    /* Read Secure Configuration Register (ARM32) */
    asm volatile("mrc p15, 0, %0, c1, c1, 0" : "=r" (scr));

    /* Check NS bit (Non-Secure bit) */
    return !(scr & (1 << 0));
#else
    /* Non-ARM architecture - assume normal world */
    return 0;
#endif
}

/**
 * @brief ARM SMC call assembly wrapper
 *
 * @param func_id Function ID
 * @param arg0 First argument
 * @param arg1 Second argument
 * @param arg2 Third argument
 * @param arg3 Fourth argument
 * @return SMC result
 */
static uint32_t smc_call_asm(uint32_t func_id,
                           uint32_t arg0,
                           uint32_t arg1,
                           uint32_t arg2,
                           uint32_t arg3)
{
#ifdef CONFIG_ARM64
    /* ARM64 SMC call implementation */
    register uint64_t x0 asm("x0") = func_id;
    register uint64_t x1 asm("x1") = arg0;
    register uint64_t x2 asm("x2") = arg1;
    register uint64_t x3 asm("x3") = arg2;
    register uint64_t x4 asm("x4") = arg3;

    asm volatile(
        "smc #0\n"
        : "+r" (x0), "+r" (x1), "+r" (x2), "+r" (x3), "+r" (x4)
        :
        : "memory"
    );

    return (uint32_t)x0;
#elif defined(CONFIG_ARM)
    /* ARM32 SMC call implementation */
    register uint32_t r0 asm("r0") = func_id;
    register uint32_t r1 asm("r1") = arg0;
    register uint32_t r2 asm("r2") = arg1;
    register uint32_t r3 asm("r3") = arg2;
    register uint32_t r4 asm("r4") = arg3;

    asm volatile(
        "smc #0\n"
        : "+r" (r0), "+r" (r1), "+r" (r2), "+r" (r3), "+r" (r4)
        :
        : "memory"
    );

    return r0;
#else
    /* Architecture not supported - return error */
    (void)func_id; (void)arg0; (void)arg1; (void)arg2; (void)arg3;
    return TZ_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Validate SMC call parameters
 *
 * @param function_id Function ID to validate
 * @param args Arguments pointer
 * @return 0 if valid, negative error code otherwise
 */
static int validate_smc_params(uint32_t function_id, void *args)
{
    /* Validate function ID range */
    if (function_id == 0 || function_id > 0xFFFF)
        return TZ_ERROR_INVALID_PARAMS;

    /* Basic args validation */
    if (!args)
        return TZ_ERROR_INVALID_PARAMS;

    return 0;
}

/**
 * @brief SMC Call implementation
 *
 * Execute Secure Monitor Call for ARM TrustZone secure world operations.
 */
int smc_call(uint32_t function_id, void *args)
{
    uint32_t result;
    int ret;

    /* Check TrustZone platform availability */
    if (!trustZone_supported())
        return TZ_ERROR_NOT_SUPPORTED;

    /* Validate parameters */
    ret = validate_smc_params(function_id, args);
    if (ret != 0)
        return ret;

    /* Prepare SMC call */
    uint32_t smc_func = SMC_CALL_NUM | function_id;

    /* For this implementation, we'll use a simple argument structure */
    /* In a real implementation, args would be parsed based on function_id */
    uint32_t arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;

    if (args) {
        /* Simple argument extraction - in practice this would be more complex */
        uint32_t *arg_ptr = (uint32_t *)args;
        arg0 = arg_ptr[0];
        if (arg_ptr[1]) arg1 = arg_ptr[1];
        if (arg_ptr[2]) arg2 = arg_ptr[2];
        if (arg_ptr[3]) arg3 = arg_ptr[3];
    }

    /* Execute SMC call */
    result = smc_call_asm(smc_func, arg0, arg1, arg2, arg3);

    /* Convert TrustZone status to standard error codes */
    switch (result) {
    case TZ_SUCCESS:
        return 0;
    case TZ_ERROR_INVALID_PARAMS:
        return -EINVAL;
    case TZ_ERROR_NOT_SUPPORTED:
        return -ENOTSUP;
    case TZ_ERROR_BUSY:
        return -EBUSY;
    case TZ_ERROR_TIMEOUT:
        return -ETIMEDOUT;
    case TZ_ERROR_ACCESS_DENIED:
        return -EACCES;
    default:
        return -EIO;
    }
}

/**
 * @brief Check if SMC platform is available
 *
 * @return 1 if available, 0 otherwise
 */
static int smc_platform_available(void)
{
    return trustZone_supported() && !in_secure_world();
}

/**
 * @brief Get SMC capabilities
 *
 * @param capabilities Pointer to capabilities structure
 * @return 0 on success, negative error code on failure
 */
static int get_smc_capabilities(void *capabilities)
{
    /* SMC capabilities would include:
     * - Available secure services
     * - Crypto algorithms supported
     * - Secure storage capacity
     * - TPM functionality
     */
    return -ENOSYS; /* Not implemented yet */
}

/**
 * @brief Initialize SMC platform
 *
 * @return 0 on success, negative error code on failure
 */
static int smc_initialize(void)
{
    /* SMC initialization would involve:
     * 1. Verifying TrustZone is properly configured
     * 2. Establishing communication with secure world
     * 3. Negotiating secure services
     */

    if (!trustZone_supported())
        return -ENODEV;

    if (in_secure_world())
        return -EINVAL; /* Can't initialize SMC from secure world */

    return 0;
}

/**
 * @brief Cleanup SMC platform resources
 *
 * @return 0 on success, negative error code on failure
 */
static int smc_cleanup(void)
{
    /* SMC cleanup would involve:
     * 1. Closing secure world communication
     * 2. Cleaning up secure contexts
     * 3. Resetting SMC state
     */

    return 0;
}

/**
 * @brief Check if enclave platform is available (SMC implementation)
 */
int dsmil_enclave_platform_available(int platform_type)
{
    if (platform_type == 1) /* SMC/TrustZone */
        return smc_platform_available();

    return 0;
}

/**
 * @brief Get enclave platform capabilities (SMC implementation)
 */
int dsmil_enclave_get_capabilities(int platform_type, void *capabilities)
{
    if (platform_type == 1) /* SMC/TrustZone */
        return get_smc_capabilities(capabilities);

    return -EINVAL;
}

/**
 * @brief Initialize enclave platform (SMC implementation)
 */
int dsmil_enclave_initialize(int platform_type)
{
    if (platform_type == 1) /* SMC/TrustZone */
        return smc_initialize();

    return -EINVAL;
}

/**
 * @brief Cleanup enclave platform resources (SMC implementation)
 */
int dsmil_enclave_cleanup(int platform_type)
{
    if (platform_type == 1) /* SMC/TrustZone */
        return smc_cleanup();

    return -EINVAL;
}

/*
 * SMC Call Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
