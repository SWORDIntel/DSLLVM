/**
 * @file dsmil_nuclear_surety_runtime.h
 * @brief Public interface for DSMIL Nuclear Surety runtime.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_NUCLEAR_SURETY_RUNTIME_H
#define DSMIL_NUCLEAR_SURETY_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MLDSA87_PUBLIC_KEY_BYTES 2592
#define MLDSA87_SIGNATURE_BYTES 4595

typedef struct {
    char key_id[64];
    uint8_t public_key[MLDSA87_PUBLIC_KEY_BYTES];
    uint8_t signature[MLDSA87_SIGNATURE_BYTES];
    uint64_t timestamp_ns;
    bool verified;
} dsmil_approval_authority_t;

typedef struct {
    char function_name[128];
    dsmil_approval_authority_t authority1;
    dsmil_approval_authority_t authority2;
    uint64_t authorization_timestamp_ns;
    bool authorized;
} dsmil_2pi_authorization_t;

int dsmil_nuclear_surety_init(const char *officer1_id,
                              const uint8_t *officer1_pubkey,
                              const char *officer2_id,
                              const uint8_t *officer2_pubkey);

int dsmil_two_person_verify(const char *function_name,
                            const uint8_t *signature1,
                            const uint8_t *signature2,
                            const char *key_id1,
                            const char *key_id2);

bool dsmil_nc3_runtime_check(void);
void dsmil_nc3_audit_log(const char *message);

int dsmil_get_2pi_history(dsmil_2pi_authorization_t *authorizations,
                          size_t max_count);

void dsmil_nc3_get_stats(uint64_t *requests, uint64_t *granted,
                         uint64_t *denied, uint64_t *tampering);

void dsmil_nuclear_surety_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_NUCLEAR_SURETY_RUNTIME_H */
