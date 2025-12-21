/*
 * DSMIL VM Function Call Runtime Implementation
 *
 * This file implements the VM function call functionality for
 * AMD SEV (Secure Encrypted Virtualization) secure VM operations.
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
#include <asm/cpu_device_id.h>

/* AMD SEV CPUID leaf and bit definitions */
#define SEV_CPUID_LEAF 0x8000001F
#define SEV_BIT (1 << 1)  /* SEV feature bit */
#define SEV_ES_BIT (1 << 3)  /* SEV-ES feature bit */
#define SEV_SNP_BIT (1 << 4)  /* SEV-SNP feature bit */

/* AMD SEV MSR addresses */
#define MSR_AMD64_SEV 0xC0010131
#define MSR_AMD64_SEV_ES 0xC0010132

/* SEV VM function IDs */
#define SEV_FUNC_CRYPTO_OPS 0x1000
#define SEV_FUNC_ATTESTATION 0x2000
#define SEV_FUNC_SECURE_STORAGE 0x3000
#define SEV_FUNC_KEY_MANAGEMENT 0x4000

/* SEV status codes */
#define SEV_SUCCESS 0
#define SEV_ERROR_INVALID_PARAMS -1
#define SEV_ERROR_NOT_SUPPORTED -2
#define SEV_ERROR_BUSY -3
#define SEV_ERROR_TIMEOUT -4
#define SEV_ERROR_ACCESS_DENIED -5
#define SEV_ERROR_HARDWARE -6
#define SEV_ERROR_VM_NOT_ENCRYPTED -7

/* SEV instruction opcodes */
#define VMGEXIT_INSTRUCTION ".byte 0x0F, 0x01, 0xD9"

/**
 * @brief Check if AMD SEV is supported on this CPU
 *
 * @return 1 if SEV is supported, 0 otherwise
 */
static int sev_supported(void)
{
#ifdef CONFIG_X86_64
    unsigned int eax, ebx, ecx, edx;

    /* Check CPUID for SEV support */
    if (__get_cpuid_max(0x80000000, NULL) < SEV_CPUID_LEAF)
        return 0;

    __cpuid_count(SEV_CPUID_LEAF, 0, eax, ebx, ecx, edx);

    /* Check SEV feature bit */
    if (!(eax & SEV_BIT))
        return 0;

    return 1;
#else
    /* SEV not supported on non-x86 architectures */
    return 0;
#endif
}

/**
 * @brief Check if SEV-ES is supported
 *
 * @return 1 if SEV-ES is supported, 0 otherwise
 */
static int sev_es_supported(void)
{
#ifdef CONFIG_X86_64
    unsigned int eax, ebx, ecx, edx;

    if (__get_cpuid_max(0x80000000, NULL) < SEV_CPUID_LEAF)
        return 0;

    __cpuid_count(SEV_CPUID_LEAF, 0, eax, ebx, ecx, edx);

    return (eax & SEV_ES_BIT) ? 1 : 0;
#else
    /* SEV-ES not supported on non-x86 architectures */
    return 0;
#endif
}

/**
 * @brief Check if SEV-SNP is supported
 *
 * @return 1 if SEV-SNP is supported, 0 otherwise
 */
static int sev_snp_supported(void)
{
#ifdef CONFIG_X86_64
    unsigned int eax, ebx, ecx, edx;

    if (__get_cpuid_max(0x80000000, NULL) < SEV_CPUID_LEAF)
        return 0;

    __cpuid_count(SEV_CPUID_LEAF, 0, eax, ebx, ecx, edx);

    return (eax & SEV_SNP_BIT) ? 1 : 0;
#else
    /* SEV-SNP not supported on non-x86 architectures */
    return 0;
#endif
}

/**
 * @brief Check if we're running in an SEV-encrypted VM
 *
 * @return 1 if in SEV VM, 0 otherwise
 */
static int in_sev_vm(void)
{
#ifdef CONFIG_X86_64
    uint64_t sev_status;

    /* Read SEV status MSR */
    if (rdmsrl_safe(MSR_AMD64_SEV, &sev_status))
        return 0;

    /* Check if SEV is enabled */
    return (sev_status & 1) ? 1 : 0;
#else
    /* SEV not supported on non-x86 architectures */
    return 0;
#endif
}

/**
 * @brief AMD VMGEXIT call for SEV-ES communication
 *
 * @param function_id Function ID for the secure VM call
 * @param args Pointer to arguments
 * @return SEV status code
 */
static uint32_t sev_vmgexit_call(uint32_t function_id, void *args)
{
#ifdef CONFIG_X86_64
    uint32_t result;

    /* VMGEXIT instruction for SEV-ES communication */
    /* This is a simplified implementation - real SEV-ES would use GHCB */
    asm volatile(
        "mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        VMGEXIT_INSTRUCTION "\n"
        "mov %%eax, %0\n"
        : "=r" (result)
        : "r" (function_id), "r" (args)
        : "eax", "ebx", "memory"
    );

    return result;
#else
    /* SEV not supported on non-x86 architectures */
    (void)function_id; (void)args;
    return SEV_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Validate VM function call parameters
 *
 * @param function_id Function ID to validate
 * @param args Arguments pointer
 * @return 0 if valid, negative error code otherwise
 */
static int validate_vm_params(uint32_t function_id, void *args)
{
    /* Validate function ID range */
    if (function_id == 0 || function_id > 0xFFFF)
        return SEV_ERROR_INVALID_PARAMS;

    /* Basic args validation */
    if (!args)
        return SEV_ERROR_INVALID_PARAMS;

    return 0;
}

/**
 * @brief VM Function Call implementation
 *
 * Execute function in AMD SEV secure virtual machine environment.
 */
int vm_function_call(uint32_t function_id, void *args)
{
    uint32_t result;
    int ret;

    /* Check SEV platform availability */
    if (!sev_supported())
        return SEV_ERROR_NOT_SUPPORTED;

    if (!in_sev_vm())
        return SEV_ERROR_VM_NOT_ENCRYPTED;

    /* Validate parameters */
    ret = validate_vm_params(function_id, args);
    if (ret != 0)
        return ret;

    /* Execute VM call based on SEV capabilities */
    if (sev_es_supported()) {
        /* Use VMGEXIT for SEV-ES */
        result = sev_vmgexit_call(function_id, args);
    } else {
        /* Fallback for basic SEV - would use hypercall or other mechanism */
        /* For now, return not supported */
        return SEV_ERROR_NOT_SUPPORTED;
    }

    /* Convert SEV status to standard error codes */
    switch (result) {
    case SEV_SUCCESS:
        return 0;
    case SEV_ERROR_INVALID_PARAMS:
        return -EINVAL;
    case SEV_ERROR_NOT_SUPPORTED:
        return -ENOTSUP;
    case SEV_ERROR_BUSY:
        return -EBUSY;
    case SEV_ERROR_TIMEOUT:
        return -ETIMEDOUT;
    case SEV_ERROR_ACCESS_DENIED:
        return -EACCES;
    case SEV_ERROR_HARDWARE:
        return -EIO;
    case SEV_ERROR_VM_NOT_ENCRYPTED:
        return -ENODEV;
    default:
        return -EIO;
    }
}

/**
 * @brief Check if VM function call platform is available
 *
 * @return 1 if available, 0 otherwise
 */
static int vm_platform_available(void)
{
    return sev_supported() && in_sev_vm();
}

/**
 * @brief Get VM/SEV capabilities
 *
 * @param capabilities Pointer to capabilities structure
 * @return 0 on success, negative error code on failure
 */
static int get_vm_capabilities(void *capabilities)
{
    /* SEV capabilities would include:
     * - SEV/SEV-ES/SEV-SNP support
     * - Available secure services
     * - Crypto algorithms supported
     * - Attestation capabilities
     */
    return -ENOSYS; /* Not implemented yet */
}

/**
 * @brief Initialize VM/SEV platform
 *
 * @return 0 on success, negative error code on failure
 */
static int vm_initialize(void)
{
    /* VM/SEV initialization would involve:
     * 1. Verifying SEV is properly configured
     * 2. Establishing communication with SEV firmware
     * 3. Setting up secure VM contexts
     * 4. Initializing attestation services
     */

    if (!sev_supported())
        return -ENODEV;

    if (!in_sev_vm())
        return -EINVAL; /* Not running in SEV VM */

    return 0;
}

/**
 * @brief Cleanup VM/SEV platform resources
 *
 * @return 0 on success, negative error code on failure
 */
static int vm_cleanup(void)
{
    /* VM/SEV cleanup would involve:
     * 1. Destroying secure VM contexts
     * 2. Cleaning up attestation state
     * 3. Resetting SEV communication
     */

    return 0;
}

/**
 * @brief Check if enclave platform is available (VM implementation)
 */
int dsmil_enclave_platform_available(int platform_type)
{
    if (platform_type == 2) /* VM/SEV */
        return vm_platform_available();

    return 0;
}

/**
 * @brief Get enclave platform capabilities (VM implementation)
 */
int dsmil_enclave_get_capabilities(int platform_type, void *capabilities)
{
    if (platform_type == 2) /* VM/SEV */
        return get_vm_capabilities(capabilities);

    return -EINVAL;
}

/**
 * @brief Initialize enclave platform (VM implementation)
 */
int dsmil_enclave_initialize(int platform_type)
{
    if (platform_type == 2) /* VM/SEV */
        return vm_initialize();

    return -EINVAL;
}

/**
 * @brief Cleanup enclave platform resources (VM implementation)
 */
int dsmil_enclave_cleanup(int platform_type)
{
    if (platform_type == 2) /* VM/SEV */
        return vm_cleanup();

    return -EINVAL;
}

/*
 * VM Function Call Runtime - Part of DSMIL Runtime Library
 * Author: DSMIL Development Team
 * Version: 1.0
 */
