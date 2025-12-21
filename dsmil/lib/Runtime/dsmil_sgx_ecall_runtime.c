/*
 * DSMIL SGX ECALL Runtime Implementation
 *
 * This file implements the SGX ECALL functionality for secure execution
 * within Intel SGX enclaves.
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
#include <asm/intel-family.h>

/* SGX CPUID leaf and sub-leaf values */
#define SGX_CPUID_LEAF 0x12
#define SGX_CPUID_SUBLEAF_MAX_ENUMERATIONS 0x0
#define SGX_CPUID_SUBLEAF_EPC_SECTION 0x1

/* SGX MSR addresses */
#define MSR_IA32_FEATURE_CONTROL 0x3A
#define MSR_IA32_SGXLEPUBKEYHASH0 0x8C
#define MSR_IA32_SGXLEPUBKEYHASH1 0x8D
#define MSR_IA32_SGXLEPUBKEYHASH2 0x8E
#define MSR_IA32_SGXLEPUBKEYHASH3 0x8F

/* SGX instruction opcodes */
#define ENCLS_ENCLU_LEAF 0x0D
#define EENTER_LEAF 0x02
#define EEXIT_LEAF 0x04

/* Forward declarations for SGX assembly functions */
extern sgx_status_t sgx_enter_enclave(uint32_t function_id,
                                     void *input, size_t input_size,
                                     void *output, size_t *output_size);

/**
 * @brief Check if SGX is supported on this CPU
 *
 * @return 1 if SGX is supported, 0 otherwise
 */
static int sgx_supported(void)
{
    unsigned int eax, ebx, ecx, edx;

    /* Check CPUID for SGX support */
    if (__get_cpuid_max(0, NULL) < SGX_CPUID_LEAF)
        return 0;

    __cpuid_count(SGX_CPUID_LEAF, SGX_CPUID_SUBLEAF_MAX_ENUMERATIONS,
                  eax, ebx, ecx, edx);

    /* Check if SGX is available */
    if (!(eax & (1 << 0)))
        return 0;

    /* Check if SGX1 is supported */
    if (!(eax & (1 << 1)))
        return 0;

    return 1;
}

/**
 * @brief Check if SGX is enabled in BIOS
 *
 * @return 1 if SGX is enabled, 0 otherwise
 */
static int sgx_enabled(void)
{
    uint64_t msr_value;

    /* Read FEATURE_CONTROL MSR */
    if (rdmsrl_safe(MSR_IA32_FEATURE_CONTROL, &msr_value))
        return 0;

    /* Check if SGX is globally enabled */
    if (!(msr_value & (1ULL << 18)))
        return 0;

    /* Check if SGX launch control is locked */
    if (!(msr_value & (1ULL << 17)))
        return 0;

    return 1;
}

/**
 * @brief Validate ECALL parameters
 *
 * @param function_id Function ID to validate
 * @param input Input buffer
 * @param input_size Input size
 * @param output Output buffer
 * @param output_size Output size pointer
 * @return SGX_SUCCESS if valid, error code otherwise
 */
static sgx_status_t validate_ecall_params(uint32_t function_id,
                                         const void *input,
                                         size_t input_size,
                                         void *output,
                                         size_t *output_size)
{
    /* Validate function ID range */
    if (function_id == 0 || function_id > 0xFFFF)
        return SGX_ERROR_INVALID_FUNCTION;

    /* Validate input parameters */
    if (input_size > 0 && !input)
        return SGX_ERROR_INVALID_PARAMETER;

    if (input_size == 0 && input)
        return SGX_ERROR_INVALID_PARAMETER;

    /* Validate output parameters */
    if (output && !output_size)
        return SGX_ERROR_INVALID_PARAMETER;

    if (!output && output_size)
        return SGX_ERROR_INVALID_PARAMETER;

    return SGX_SUCCESS;
}

/**
 * @brief SGX ECALL implementation
 *
 * Execute function in Intel SGX enclave with secure execution guarantees.
 */
sgx_status_t sgx_ecall(uint32_t function_id,
                      const void *input,
                      size_t input_size,
                      void *output,
                      size_t *output_size)
{
    sgx_status_t status;

    /* Check SGX platform availability */
    if (!sgx_supported())
        return SGX_ERROR_INVALID_STATE;

    if (!sgx_enabled())
        return SGX_ERROR_INVALID_STATE;

    /* Validate parameters */
    status = validate_ecall_params(function_id, input, input_size,
                                  output, output_size);
    if (status != SGX_SUCCESS)
        return status;

    /* Execute the ECALL */
    return sgx_enter_enclave(function_id, (void *)input, input_size,
                            output, output_size);
}

/**
 * @brief Check if SGX platform is available
 *
 * @return 1 if available, 0 otherwise
 */
int dsmil_enclave_platform_available(int platform_type)
{
    if (platform_type == 0) /* SGX */
        return sgx_supported() && sgx_enabled();

    /* Other platforms not implemented yet */
    return 0;
}

/**
 * @brief Get SGX capabilities
 *
 * @param capabilities Pointer to capabilities structure
 * @return 0 on success, negative error code on failure
 */
static int get_sgx_capabilities(void *capabilities)
{
    /* SGX capabilities structure would be defined elsewhere */
    /* For now, return not implemented */
    return -ENOSYS;
}

/**
 * @brief Get enclave platform capabilities
 */
int dsmil_enclave_get_capabilities(int platform_type, void *capabilities)
{
    if (platform_type == 0) /* SGX */
        return get_sgx_capabilities(capabilities);

    return -EINVAL;
}

/**
 * @brief Initialize SGX platform
 *
 * @return 0 on success, negative error code on failure
 */
static int sgx_initialize(void)
{
    /* SGX initialization would involve:
     * 1. Checking/enabling SGX in BIOS (if possible)
     * 2. Loading SGX kernel module
     * 3. Setting up EPC pages
     * 4. Initializing enclave contexts
     */

    /* For now, just verify SGX is available */
    if (!sgx_supported() || !sgx_enabled())
        return -ENODEV;

    return 0;
}

/**
 * @brief Initialize enclave platform
 */
int dsmil_enclave_initialize(int platform_type)
{
    if (platform_type == 0) /* SGX */
        return sgx_initialize();

    return -EINVAL;
}

/**
 * @brief Cleanup SGX platform resources
 *
 * @return 0 on success, negative error code on failure
 */
static int sgx_cleanup(void)
{
    /* SGX cleanup would involve:
     * 1. Destroying enclave contexts
     * 2. Freeing EPC pages
     * 3. Unloading SGX resources
     */

    return 0;
}

/**
 * @brief Cleanup enclave platform resources
 */
int dsmil_enclave_cleanup(int platform_type)
{
    if (platform_type == 0) /* SGX */
        return sgx_cleanup();

    return -EINVAL;
}

/*
 * SGX ECALL Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
