/*
 * DSMIL Enclave APIs Header
 *
 * This header defines the enclave API functions for secure execution
 * environments including Intel SGX, ARM TrustZone, and AMD SEV.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_ENCLAVE_APIS_H
#define DSMIL_ENCLAVE_APIS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * SGX ECALL API
 * ============================================================================ */

/**
 * @brief SGX status return codes
 */
typedef enum {
    SGX_SUCCESS = 0,
    SGX_ERROR_INVALID_PARAMETER = -1,
    SGX_ERROR_OUT_OF_MEMORY = -2,
    SGX_ERROR_UNEXPECTED = -3,
    SGX_ERROR_INVALID_STATE = -4,
    SGX_ERROR_INVALID_FUNCTION = -5,
    SGX_ERROR_OUT_OF_EPC = -6,
    SGX_ERROR_ENCLAVE_LOST = -7,
    SGX_ERROR_INVALID_ENCLAVE = -8,
    SGX_ERROR_UNDEFINED_SYMBOL = -9,
    SGX_ERROR_INVALID_ENCLAVE_ID = -10,
    SGX_ERROR_INVALID_SIGNATURE = -11,
    SGX_ERROR_NDEBUG_ENCLAVE = -12,
    SGX_ERROR_OUT_OF_TCS = -13,
    SGX_ERROR_ENCLAVE_CRASHED = -14,
    SGX_ERROR_ECALL_NOT_ALLOWED = -15,
    SGX_ERROR_OCALL_NOT_ALLOWED = -16,
    SGX_ERROR_STACK_OVERRUN = -17
} sgx_status_t;

/**
 * @brief Execute function in Intel SGX enclave
 *
 * This function executes a specified function within an Intel SGX enclave
 * with secure execution guarantees.
 *
 * @param function_id The ID of the function to execute within the enclave
 * @param input Pointer to input data buffer
 * @param input_size Size of input data in bytes
 * @param output Pointer to output data buffer (may be NULL)
 * @param output_size Pointer to output size (updated with actual size)
 * @return SGX_SUCCESS on success, error code otherwise
 */
sgx_status_t sgx_ecall(uint32_t function_id,
                      const void *input,
                      size_t input_size,
                      void *output,
                      size_t *output_size);

/* ============================================================================
 * SMC CALL API (ARM TrustZone)
 * ============================================================================ */

/**
 * @brief Execute Secure Monitor Call for ARM TrustZone
 *
 * This function performs a secure monitor call to execute operations
 * in the ARM TrustZone secure world.
 *
 * @param function_id The secure function ID to execute
 * @param args Pointer to arguments structure (platform-specific)
 * @return 0 on success, negative error code on failure
 */
int smc_call(uint32_t function_id, void *args);

/* ============================================================================
 * VM FUNCTION CALL API (AMD SEV)
 * ============================================================================ */

/**
 * @brief Execute function in AMD SEV secure VM
 *
 * This function executes a specified function within an AMD SEV
 * secure virtual machine environment.
 *
 * @param function_id The ID of the function to execute in the VM
 * @param args Pointer to arguments structure (platform-specific)
 * @return 0 on success, negative error code on failure
 */
int vm_function_call(uint32_t function_id, void *args);

/* ============================================================================
 * COMMON ENCLAVE UTILITIES
 * ============================================================================ */

/**
 * @brief Check if enclave platform is available
 *
 * @param platform_type Type of enclave platform (0=SGX, 1=SMC, 2=VM)
 * @return 1 if available, 0 if not available
 */
int dsmil_enclave_platform_available(int platform_type);

/**
 * @brief Get enclave platform capabilities
 *
 * @param platform_type Type of enclave platform
 * @param capabilities Pointer to capabilities structure (output)
 * @return 0 on success, negative error code on failure
 */
int dsmil_enclave_get_capabilities(int platform_type, void *capabilities);

/**
 * @brief Initialize enclave platform
 *
 * @param platform_type Type of enclave platform to initialize
 * @return 0 on success, negative error code on failure
 */
int dsmil_enclave_initialize(int platform_type);

/**
 * @brief Cleanup enclave platform resources
 *
 * @param platform_type Type of enclave platform to cleanup
 * @return 0 on success, negative error code on failure
 */
int dsmil_enclave_cleanup(int platform_type);

#endif /* DSMIL_ENCLAVE_APIS_H */
